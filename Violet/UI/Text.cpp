#include "stdafx.h"
#include "Text.hpp"
#include "Core/Resource.hpp"
#include "Rendering/Texture.hpp"
#include "File/Filesystem.hpp"
#include "PixelDraw.hpp"
#include "Layout.hpp"
#include "Utils/Profiling.hpp"

#include <array>
#include <cstdint>
#include <codecvt>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"

#include "NativeText/native_text.h"

#include <iostream>

using namespace UI;

static const int TEX_SIZE = 512;

struct TextGen
{
    TextGen();
    
    TypedTex<RGBA8Px> tex;
    std::unordered_map<std::string, Eigen::AlignedBox2f> locs;
    
    stbrp_context stbrp;
    std::array<stbrp_node, TEX_SIZE> stbrp_temp;
    
    bool PackStrings(std::vector<std::string>& strs);
    void Clear();
};

struct TextLoc
{
    Vector2i loc;
    int z;
};

struct TextContext
{
    TextContext();
    ~TextContext();
    TextGen youngGen, oldGen;
    std::unordered_map<std::string, std::vector<TextLoc>> toDraw;
    native_text* nt;
    RGBA8Px texBuf[TEX_SIZE * TEX_SIZE];
    float scaling;
};

TextContext& GetTextContext()
{
    static TextContext ctx;
    return ctx;
}

TextGen::TextGen()
    : tex(TexDim{TEX_SIZE, TEX_SIZE})
{
    Clear();
}

TextContext::TextContext()
    : nt(nt_init()), scaling(1)
{
    nt_buffer(nt, (char*)texBuf, TEX_SIZE, TEX_SIZE, 4);
    nt_color(nt,
             float(Colors::fg >> 24 & 0xFF) / 255.f,
             float(Colors::fg >> 16 & 0xFF) / 255.f,
             float(Colors::fg >>  8 & 0xFF) / 255.f,
             float(Colors::fg >>  0 & 0xFF) / 255.f);
}

TextContext::~TextContext()
{
    nt_free(nt);
}

Vector2i UI::TextDim(const std::string& text)
{
	return TextDim(text.begin(), text.end());
}

Vector2i UI::TextDim(std::string::const_iterator begin, std::string::const_iterator end)
{
    nt_extent ext = nt_get_extent(GetTextContext().nt, &*begin, end - begin);
    
    return{ ext.w, ext.h };
}

void UI::DrawText(const std::string& text, AlignedBox2i container, TextAlign align)
{
	Vector2i dim{ TextDim(text).x(), LINEH }; //doesn't give useful y dimension
	Vector2i space = (container.sizes() - dim) / 2;
	if (align == TextAlign::Left) space.x() = TEXT_PADDING;
	if (align == TextAlign::Right) space.x() = container.sizes().x() - dim.x() - TEXT_PADDING;
	//UI::DrawBox(container);
	DrawText(text, container.corner(AlignedBox2i::BottomLeft) + space);
}

void UI::DrawText(const std::string& text, Vector2i pos)
{
    GetTextContext().toDraw[text].push_back({pos, CurZ()});
}

bool TextGen::PackStrings(std::vector<std::string>& strs)
{
    float s = GetTextContext().scaling;
    
    std::vector<stbrp_rect> toPack(strs.size());
    for (size_t i = 0; i < strs.size(); ++i)
    {
        auto sz = TextDim(strs[i]) * s; //do packing in pixel coords
        toPack[i].w = sz.x();
        toPack[i].h = sz.y();
    }
    
    stbrp_pack_rects(&stbrp, &toPack[0], static_cast<int>(toPack.size()));
    
    for (const auto& rect : toPack)
        if (!rect.was_packed)
            return false;

    for (auto& px : GetTextContext().texBuf) px = {0, 0, 0, 0};
    
    for (size_t i = 0; i < strs.size(); ++i)
    {
        nt_put_text(GetTextContext().nt, strs[i].c_str(), strs[i].size(), toPack[i].x / s, toPack[i].y / s, nullptr);
        
        //scale the box into pixel coords from screen coords
        TexBox box = {TexDim{toPack[i].x, toPack[i].y}, TexDim{toPack[i].x + toPack[i].w, toPack[i].y + toPack[i].h}};
        
        //set tex coords
        locs[strs[i]] = {box.min().cast<float>() / float{TEX_SIZE}, box.max().cast<float>() / float{TEX_SIZE}};
        
        //do individual pixel transfers so as not to clobber existing data
        tex.SubImageOf(GetTextContext().texBuf, box);
    }
        
    return true;
}

void TextGen::Clear()
{
    locs.clear();
    stbrp_init_target(&stbrp, TEX_SIZE, TEX_SIZE, &stbrp_temp[0], TEX_SIZE);
}

void UI::DrawAllText()
{
    auto p = Profile::Profile("text rendering");
    
    auto& toDraw = GetTextContext().toDraw;
    auto& young = GetTextContext().youngGen;
    auto& old = GetTextContext().oldGen;
    
    //Don't draw empty strings
    toDraw.erase("");
    
    //Gather up the strings that we haven't drawn yet
    std::vector<std::string> newStrs;
    for (const auto& val : toDraw)
        if (!young.locs.count(val.first)
            && !old.locs.count(val.first))
            newStrs.push_back(val.first);
    
    if (newStrs.size() && !young.PackStrings(newStrs))
    {
        //Texture is full, GC young. Put all the strings currently in young into old
        std::vector<std::string> toOld;
        for (const auto& p : young.locs)
            if (toDraw.count(p.first)) //still needed?
                toOld.push_back(p.first);
        
        //std::cerr << "Young GC, keeping " << toOld.size() << " of " << young.locs.size() << '\n';
        
        //now there should be a place in young for the new strings
        young.Clear();
        if (!young.PackStrings(newStrs))
            throw std::runtime_error("Out of text space");
        
        if (!old.PackStrings(toOld))
        {
            //Texture is full, GC old.
            for (const auto& p : old.locs)
                if (toDraw.count(p.first))
                    toOld.push_back(p.first);
            
            //std::cerr << "Old GC, keeping " << toOld.size() << " of " << old.locs.size() << '\n';
            
            //Repack everything
            old.Clear();
            if (!old.PackStrings(toOld))
                throw std::runtime_error("Out of text space");
        }
    }
    
    //All the text is drawn, so render the quads now
    for (const auto& str : toDraw)
    {
        Vector2i dim = TextDim(str.first);
        auto it = young.locs.find(str.first);
        if (it != young.locs.end())
            for (const auto& loc : str.second)
                DrawQuad(young.tex, {loc.loc, loc.loc + dim}, it->second, loc.z);
        else
        {
            it = old.locs.find(str.first);
            for (const auto& loc : str.second)
                DrawQuad(old.tex, {loc.loc, loc.loc + dim}, it->second, loc.z);
        }
    }
    
    //draw the whole texture for debugging
    //DrawQuad(young.tex, {Vector2i{0, 0}, Vector2i{512, 512}});
    
    toDraw.clear();
}

void UI::TextScaling(float scaling)
{
    if (scaling != GetTextContext().scaling)
    {
        GetTextContext().scaling = scaling;
        nt_scaling(GetTextContext().nt, scaling, scaling);
        GetTextContext().youngGen.Clear();
        GetTextContext().oldGen.Clear();
    }
}

#include "stb/stb_textedit.h"

float STB_TEXTEDIT_CHARPOS(std::u32string* str32, int n, int i)
{
    auto end = std::find(str32->begin() + n, str32->end(), U'\n');
    
    u8_u32_convert u8_to_u32;
    std::string str = u8_to_u32.to_bytes(&(*str32)[n], &(*end));
    return nt_visual_offset(GetTextContext().nt, str.c_str(), str.size(), i);
}

int STB_TEXTEDIT_HITTEST(std::u32string* str32, int n, float x)
{
    auto end = std::find(str32->begin() + n, str32->end(), U'\n');
    
    u8_u32_convert u8_to_u32;
    std::string str = u8_to_u32.to_bytes(&(*str32)[n], &(*end));
    return static_cast<int>(nt_hit_test(GetTextContext().nt, str.c_str(), str.size(), x));
}

float STB_TEXTEDIT_GETWIDTH(std::u32string* str32, int n, int i)
{
    auto nt = GetTextContext().nt;
    u8_u32_convert u8_to_u32;
    
    //width of char is difference in line length with and without it
    std::string strlong = u8_to_u32.to_bytes(&(*str32)[n], &(*str32)[n + i + 1]);
    std::string strshort = u8_to_u32.to_bytes(&(*str32)[n], &(*str32)[n + i]);
    
    return nt_get_extent(nt, strlong.c_str(), strlong.size()).w
        - nt_get_extent(nt, strshort.c_str(), strshort.size()).w;
}

void STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, std::u32string* str32, int n)
{
    r->baseline_y_delta = LINEH;
	r->x1 = r->x0 = 0;

	r->num_chars = 0;
	r->ymin = r->ymax = 0.f;
    
    auto end = std::find(str32->begin() + n, str32->end(), U'\n');
    r->num_chars = static_cast<int>(end - str32->begin()) - n;
    
    u8_u32_convert u8_to_u32;
    std::string str = u8_to_u32.to_bytes(&(*str32)[n], &(*end));
    nt_extent ext = nt_get_extent(GetTextContext().nt, str.c_str(), str.size());
    r->ymin = -ext.h;
    r->x1 = ext.w;
}