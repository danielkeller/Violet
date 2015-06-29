#include "stdafx.h"
#include "Text.hpp"
#include "Core/Resource.hpp"
#include "Rendering/Texture.hpp"
#include "File/Filesystem.hpp"
#include "PixelDraw.hpp"
#include "Layout.hpp"

#include <cstdint>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"
#include "stb/stb_truetype.h"

using namespace UI;

struct Font::FontResource : public Resource<Font::FontResource>
{
    FontResource(std::string key, std::string path, Vector2i scaling);

    int scale;
	TypedTex<std::uint8_t> texture;
	stbtt_packedchar cdata[96]; // ASCII 32..126 is 95 glyphs
};

Font::FontResource::FontResource(std::string key, std::string path, Vector2i scaling)
	: ResourceTy(key), scale(scaling.y()), texture(TexDim{512, 512})
{
	MappedFile ttf(path);
	std::uint8_t temp_bitmap[512 * 512];

	stbtt_pack_context ctx;
	stbtt_PackBegin(&ctx, temp_bitmap, 512, 512, 0, 1, nullptr);
	stbtt_PackSetOversampling(&ctx, 3, 1);
	stbtt_PackFontRange(&ctx, ttf.Data<unsigned char>(), 0, 14.f*scale, 32, 96, cdata);
	stbtt_PackEnd(&ctx);
	texture.Image(temp_bitmap);
}

Font::Font(std::string path, Vector2i scaling)
{
    std::string key = path + '_' + to_string(scaling.x()) + 'x' + to_string(scaling.y());
    if (!(resource = FontResource::FindResource(key)))
        resource = FontResource::MakeShared(key, path, scaling);
}

std::string Font::Name()
{
	return resource->Key();
}

void Font::Bind()
{
	resource->texture.Bind(0);
}

Vector2i UI::TextDim(const std::string& text)
{
	return TextDim(text.begin(), text.end());
}

Vector2i UI::TextDim(std::string::const_iterator begin, std::string::const_iterator end)
{
    auto& font = *GetFont().resource;
	Vector2f posf(0, 0);
	for (; begin != end; ++begin)
	{
		auto& cdata = font.cdata[*begin - 32];
		posf.x() += cdata.xadvance;
		posf.y() = std::max(posf.y(), cdata.yoff2 - cdata.yoff);
	}
    return posf.cast<int>() / font.scale;
}

void UI::DrawText(const std::string& text, AlignedBox2i container, TextAlign align)
{
	Vector2i dim{ TextDim(text).x(), LINEH }; //doesn't give useful y dimension
	Vector2i space = (container.sizes() - dim) / 2;
	space.y() += BASELINE_HEIGHT; //start at the correct point
	if (align == TextAlign::Left) space.x() = TEXT_PADDING;
	if (align == TextAlign::Right) space.x() = container.sizes().x() - dim.x() - TEXT_PADDING;
	//UI::DrawBox(container);
	DrawText(text, container.corner(AlignedBox2i::BottomLeft) + space);
}

void UI::DrawText(const std::string& text, Vector2i pos)
{
    Vector2f posf = Vector2f::Zero();
	//the coordinate system is backwards wrt ours so use negative y coords
	posf.y() *= -1;
    
    auto& font = *GetFont().resource;
	auto& cdata = font.cdata;
	for (int character : text)
	{
		stbtt_aligned_quad q;
		stbtt_GetPackedQuad(cdata, 512, 512, character - 32,
			&posf.x(), &posf.y(), &q, 0);
        
        Eigen::AlignedBox2f dispBox{
            Vector2f{ q.x0, -q.y0 } / font.scale,
            Vector2f{ q.x1, -q.y1 } / font.scale};

		DrawChar(
			dispBox.translate(pos.cast<float>()),
			{ Vector2f{ q.s0, q.t0 }, Vector2f{ q.s1, q.t1 } });
	}
}

#include "stb/stb_textedit.h"
#include "Layout.hpp"

float STB_TEXTEDIT_GETWIDTH(std::string* str, int n, int i)
{
	return GetFont().resource->cdata[str->at(i) - 32].xadvance
        / GetFont().resource->scale;
}

void STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, std::string* str, int n)
{
    int scale = GetFont().resource->scale;
	auto& cdata = GetFont().resource->cdata;

	r->baseline_y_delta = LINEH;
	r->x1 = r->x0 = 0;

	r->num_chars = 0;
	r->ymin = r->ymax = 0.f;

	for (auto it = str->begin() + n;
		it != str->end() && *it != '\n';
		++it)
	{
		++r->num_chars;
		r->ymin = std::min(r->ymin, cdata[*it - 32].yoff / scale);
		r->ymax = std::min(r->ymax, cdata[*it - 32].yoff2 / scale);
		r->x1 += cdata[*it - 32].xadvance / scale;
	}
}