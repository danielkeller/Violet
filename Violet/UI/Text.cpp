#include "stdafx.h"
#include "Text.hpp"
#include "Resource.hpp"
#include "Rendering/Texture.hpp"
#include "MappedFile.hpp"
#include "PixelDraw.hpp"

#include <cstdint>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"
#include "stb/stb_truetype.h"

using namespace UI;

struct Font::FontResource : public Resource<Font::FontResource>
{
	FontResource(std::string path);

	TypedTex<std::uint8_t> texture;
	stbtt_packedchar cdata[96]; // ASCII 32..126 is 95 glyphs
};

Font::FontResource::FontResource(std::string path)
	: ResourceTy(path), texture(TexDim{512, 512})
{
	MappedFile ttf(path);
	std::uint8_t temp_bitmap[512 * 512];

	stbtt_pack_context ctx;
	stbtt_PackBegin(&ctx, temp_bitmap, 512, 512, 0, 1, nullptr);
	stbtt_PackSetOversampling(&ctx, 3, 1);
	stbtt_PackFontRange(&ctx, ttf.Data<unsigned char>(), 0, 14.f, 32, 96, cdata);
	stbtt_PackEnd(&ctx);
	texture.Image(temp_bitmap);
}

Font::Font(std::string path)
	: resource(FontResource::FindOrMake(path))
{}

Font::Font()
	: Font("assets/DroidSansMono.ttf")
{}

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
	Vector2f posf(0, 0);
	for (; begin != end; ++begin)
	{
		auto& cdata = GetFont().resource->cdata[*begin - 32];
		posf.x() += cdata.xadvance;
		posf.y() = std::max(posf.y(), cdata.yoff2 - cdata.yoff);
	}
	return posf.cast<int>();
}

void UI::DrawText(const std::string& text, AlignedBox2i container)
{
	Vector2i dim = TextDim(text);
	Vector2i space = (container.sizes() - dim) / 2;
	space.y() = -space.y();
	DrawText(text, container.corner(AlignedBox2i::TopLeft) + space);
}

void UI::DrawText(const std::string& text, Vector2i pos)
{
	Vector2f posf = pos.cast<float>();

	auto& cdata = GetFont().resource->cdata;
	for (int character : text)
	{
		stbtt_aligned_quad q;
		stbtt_GetPackedQuad(cdata, 512, 512, character - 32,
			&posf.x(), &posf.y(), &q, 1);

		TextQuad tq{
			{ Vector2f{ q.x0, q.y0 }, Vector2f{ q.x1, q.y1 } },
			{ Vector2f{ q.s0, q.t0 }, Vector2f{ q.s1, q.t1 } } };
		DrawChar(tq);
	}
}

#include "stb/stb_textedit.h"
#include "Layout.hpp"

float STB_TEXTEDIT_GETWIDTH(std::string* str, int n, int i)
{
	return GetFont().resource->cdata[str->at(i) - 32].xadvance;
}

void STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, std::string* str, int n)
{
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
		r->ymin = std::min(r->ymin, cdata[*it - 32].yoff);
		r->ymax = std::min(r->ymax, cdata[*it - 32].yoff2);
		r->x1 += cdata[*it - 32].xadvance;
	}
}