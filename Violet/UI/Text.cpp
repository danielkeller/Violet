#include "stdafx.h"
#include "Text.hpp"
#include "Resource.hpp"
#include "Rendering/Texture.hpp"
#include "MappedFile.hpp"

#include <cstdint>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"
#include "stb/stb_truetype.h"

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
	//stbtt_BakeFontBitmap(ttf.Data<unsigned char>(), 0, 12.0, temp_bitmap, 512, 512, 32, 96, cdata);

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

std::string Font::Name()
{
	return resource->Key();
}

#include "Rendering/VAO.hpp"
#include "Rendering/Shader.hpp"
#include "Window.hpp"
#include "PixelDraw.hpp"

template<>
const Schema AttribTraits<stbtt_aligned_quad>::schema = {
	{ "topLeft",     GL_FLOAT, false, 0,                 { 2, 1 } },
	{ "topLeftTex",  GL_FLOAT, false, 2 * sizeof(float), { 2, 1 } },
	{ "botRight",    GL_FLOAT, false, 4 * sizeof(float), { 2, 1 } },
	{ "botRightTex", GL_FLOAT, false, 6 * sizeof(float), { 2, 1 } },
};

void DrawText(const Font &font, const std::string& text, Vector2i pos, Vector3f color, Vector3f bgColor)
{
	std::vector<stbtt_aligned_quad> quads(text.size());
	auto it = quads.begin();
	Vector2f posf = pos.cast<float>();

	for (int character : text)
		stbtt_GetPackedQuad(font.resource->cdata, 512, 512, character - 32, 
			&posf.x(), &posf.y(), &*(it++), 1);

	static ShaderProgram txtShdr{ "assets/text" };
	static UBO txtUBO = txtShdr.MakeUBO("Material", "TxtMat");
	static VAO txtVAO(txtShdr, UnitBox);
	static BufferObject<stbtt_aligned_quad, GL_ARRAY_BUFFER, GL_STREAM_DRAW> charInstances;

	font.resource->texture.Bind(0);
	charInstances.Data(quads);
	txtVAO.BindInstanceData(txtShdr, charInstances);

	txtUBO["color"] = color;
	txtUBO["bgColor"] = bgColor;
	txtUBO.Sync();

	txtShdr.use();
	txtUBO.Bind();
	BindPixelUBO();

	txtVAO.Draw();
}