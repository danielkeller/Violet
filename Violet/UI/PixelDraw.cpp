#include "stdafx.h"
#include "PixelDraw.hpp"

#include "Rendering/Shader.hpp"
#include "Rendering/VAO.hpp"
#include "Window.hpp"
#include "Layout.hpp"
#include "Text.hpp"

using namespace UI;

struct UI::Settings
{
	Font font;
};

Settings& GetSettings()
{
	static Settings settings;
	return settings;
}

Font UI::GetFont()
{
	return GetSettings().font;
}

void UI::TextStyle(Font font, Vector3f color, Vector3f bgColor)
{
	GetSettings().font = font;

	static ShaderProgram txtShdr{ "assets/text" };
	static UBO txtUBO = txtShdr.MakeUBO("Material", "TxtMat");
	txtUBO["color"] = color;
	txtUBO["bgColor"] = bgColor;
}

struct TextureQuad
{
	Tex tex;
	TextQuad quad;
};

struct UI::Visuals
{
	std::vector<TextQuad> textInsts;
	std::vector<AlignedBox2i> boxes;
	std::vector<AlignedBox2i> hlBoxes;
	std::vector<TextureQuad> quads;
};

Visuals& FrameVisuals()
{
	static Visuals vis;
	return vis;
}

void UI::BeginFrame(Window& w, Events e)
{
	FrameVisuals() = {};
	CurLayout() = LayoutStack(*w.dim);
	FrameEvents() = e;
}

Events& UI::FrameEvents()
{
	static Events events;
	return events;
}

LayoutStack& UI::CurLayout()
{
	static LayoutStack layout({ 0, 0 });
	return layout;
}

void UI::EndFrame()
{
	//Draw boxes
	static ShaderProgram boxShdr("assets/uibox");
	static UBO boxUBO = boxShdr.MakeUBO("Material", "BoxMat");
	static VAO boxVAO(boxShdr, UnitBox);
	static BufferObject<AlignedBox2i, GL_ARRAY_BUFFER, GL_STREAM_DRAW> boxInstances;

	boxInstances.Data(FrameVisuals().boxes);
	boxVAO.BindInstanceData(boxShdr, boxInstances);

	boxShdr.use();
	BindPixelUBO();
	boxUBO.Bind();
	boxVAO.Draw();

	//Draw textured quads
	static ShaderProgram quadShdr{ "assets/textured_uibox" };
	static VAO quadVAO(quadShdr, UnitBox);
	static BufferObject<TextQuad, GL_ARRAY_BUFFER, GL_STREAM_DRAW> quadInstances(1);

	quadShdr.use();
	quadVAO.BindInstanceData(quadShdr, quadInstances);
	BindPixelUBO();

	for (const auto& q : FrameVisuals().quads)
	{
		quadInstances.Assign(0, q.quad);
		q.tex.Bind(0);
		quadVAO.Draw();
	}

	//Draw text
	static ShaderProgram txtShdr{ "assets/text" };
	static UBO txtUBO = txtShdr.MakeUBO("Material", "TxtMat");
	static VAO txtVAO(txtShdr, UnitBox);
	static BufferObject<TextQuad, GL_ARRAY_BUFFER, GL_STREAM_DRAW> charInstances;

	GetSettings().font.Bind();
	charInstances.Data(FrameVisuals().textInsts);
	txtVAO.BindInstanceData(txtShdr, charInstances);

	txtShdr.use();
	txtUBO.Bind();
	BindPixelUBO();

	txtVAO.Draw();

	//Draw hilight boxes
	static UBO hlboxUBO = boxShdr.MakeUBO("Material", "HlBoxMat");
	static BufferObject<AlignedBox2i, GL_ARRAY_BUFFER, GL_STREAM_DRAW> hlBoxInstances;

	hlBoxInstances.Data(FrameVisuals().hlBoxes);
	boxVAO.BindInstanceData(boxShdr, hlBoxInstances);

	boxShdr.use();
	BindPixelUBO();
	hlboxUBO.Bind();
	boxVAO.Draw();
}

void UI::DrawChar(TextQuad q)
{
	FrameVisuals().textInsts.push_back(q);
}

void UI::DrawBox(AlignedBox2i box)
{
	FrameVisuals().boxes.push_back(box);
}

void UI::DrawHlBox(AlignedBox2i box)
{
	FrameVisuals().hlBoxes.push_back(box);
}

void UI::DrawQuad(Tex t, AlignedBox2i box, Eigen::AlignedBox2f tex)
{
	FrameVisuals().quads.push_back({ t, { box.cast<float>(), tex } });
}

template<>
const Schema AttribTraits<AlignedBox2i>::schema = {
	{ "minBox", GL_INT, true, 0,               { 2, 1 } },
	{ "maxBox", GL_INT, true, 2 * sizeof(int), { 2, 1 } },
};


template<>
const Schema AttribTraits<TextQuad>::schema = {
	{ "topLeft",     GL_FLOAT, false, 0,                 { 2, 1 } },
	{ "botRight",    GL_FLOAT, false, 2 * sizeof(float), { 2, 1 } },
	{ "topLeftTex",  GL_FLOAT, false, 4 * sizeof(float), { 2, 1 } },
	{ "botRightTex", GL_FLOAT, false, 6 * sizeof(float), { 2, 1 } },
};

static void WinResize(Vector2i sz)
{
	static ShaderProgram txtShdr{ "assets/uibox" };
	static UBO pixelUBO = txtShdr.MakeUBO("Common", "PixelCommon");
	pixelUBO["pixelMat"] = PixelMat(sz);
}

void UI::Init(Window& w)
{
	//init styles
	TextStyle({});

	static ShaderProgram boxShdr("assets/uibox");

	static UBO boxUBO = boxShdr.MakeUBO("Material", "BoxMat");
	boxUBO["fill"] = Vector4f{ 1, 1, 1, 1 };
	boxUBO["stroke"] = Vector4f{ 1, 1, 1, 1 };

	static UBO hlboxUBO = boxShdr.MakeUBO("Material", "HlBoxMat");
	hlboxUBO["fill"] = Vector4f{ 0, 0, 0, 0 };
	hlboxUBO["stroke"] = hlColor;

	w.dim += make_magic(accessor<Vector2i>(&WinResize));
	WinResize(*w.dim);
}

void UI::BindPixelUBO()
{
	static ShaderProgram pxlShdr{ "assets/uibox" };
	static UBO pixelUBO = pxlShdr.MakeUBO("Common", "PixelCommon");
	pixelUBO.Bind();
}