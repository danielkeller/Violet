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
	txtUBO.Sync();
}

struct UI::Visuals
{
	std::vector<TextQuad> textInsts;
	std::vector<Eigen::AlignedBox2i> boxes;
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
	static VAO boxVAO(boxShdr, UnitBox);
	static BufferObject<Eigen::AlignedBox2i, GL_ARRAY_BUFFER, GL_STREAM_DRAW> boxInstances(1);

	boxInstances.Data(FrameVisuals().boxes);

	boxVAO.BindInstanceData(boxShdr, boxInstances);

	boxShdr.use();
	BindPixelUBO();
	boxVAO.Draw();

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
}

void UI::DrawChar(TextQuad q)
{
	FrameVisuals().textInsts.push_back(q);
}

void UI::DrawBox(const Layout& l)
{
	DrawBox({ l.pos, l.pos + l.size });
}

void UI::DrawBox(Eigen::AlignedBox2i box)
{
	//fix it if it's backwards
	//Eigen::AlignedBox2i box1 =
	//	{ box.min().cwiseMin(box.max()), box.max().cwiseMax(box.min()) };
	FrameVisuals().boxes.push_back(box);
}

template<>
const Schema AttribTraits<Eigen::AlignedBox2i>::schema = {
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
	pixelUBO.Sync();
}

void UI::Init(Window& w)
{
	TextStyle({});

	w.dim += make_magic(accessor<Vector2i>(&WinResize));
	WinResize(*w.dim);
}

void UI::BindPixelUBO()
{
	static ShaderProgram pxlShdr{ "assets/uibox" };
	static UBO pixelUBO = pxlShdr.MakeUBO("Common", "PixelCommon");
	pixelUBO.Bind();
}