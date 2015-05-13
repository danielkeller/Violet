#include "stdafx.h"
#include "PixelDraw.hpp"

#include "Rendering/Shader.hpp"
#include "Rendering/VAO.hpp"
#include "Rendering/FBO.hpp"
#include "Window.hpp"
#include "Layout.hpp"
#include "Text.hpp"

using namespace UI;

static const float maxZ = 10.f;

Matrix4f UI::PixelMat(Vector2i dim)
{
	Matrix4f ret;
	ret <<
		2.f / float(dim.x()), 0, 0, -1,
		0, 2.f / float(dim.y()), 0, -1,
		0, 0, -1.f / maxZ, .999f,
		0, 0, 0, 1;
	return ret;
}

struct UI::Settings
{
	Settings() : screenTex(TexDim{ 0, 0 }) {}
	Font font;
	Vector3f textColor;
	TypedTex<> screenTex;
	FBO fbo;
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

void UI::TextStyle(Font font, Vector3f color)
{
	GetSettings().font = font;
	GetSettings().textColor = color;
}

struct Box2z
{
	AlignedBox2i box;
	int z;
};

struct UIBox
{
	AlignedBox2i box;
	Vector4f fill, stroke;
	int z;
};

struct TextQuad
{
	Eigen::AlignedBox2f pos;
	Eigen::AlignedBox2f tex;
	int z;
};

struct TextureQuad
{
	Tex tex;
	TextQuad quad;
};

struct UI::Visuals
{
	Visuals() : zStack({ 1 }) {}
	std::vector<TextQuad> textInsts;
	std::vector<UIBox> boxes;
	std::vector<Box2z> shadows;
	std::vector<TextureQuad> quads;
	std::vector<int> zStack;
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
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	//Draw boxes
	static ShaderProgram boxShdr("assets/uibox");
	static VAO boxVAO(boxShdr, UnitBox);
	static BufferObject<UIBox, GL_ARRAY_BUFFER, GL_STREAM_DRAW> boxInstances;

	boxInstances.Data(FrameVisuals().boxes);
	//TODO: just set numInstances
	boxVAO.BindInstanceData(boxShdr, boxInstances);

	boxShdr.use();
	BindPixelUBO();

	{
		auto bound = GetSettings().fbo.Bind(GL_FRAMEBUFFER);
		GetSettings().fbo.PreDraw({ Eigen::Matrix<GLuint, 4, 1>::Zero() });
		boxVAO.Draw();
	}

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
	glEnable(GL_BLEND);
	//blend from constant (text) color to dest color by source color
	glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_COLOR);
	auto textColor = GetSettings().textColor;
	glBlendColor(textColor.x(), textColor.y(), textColor.z(), 1.f);

	static ShaderProgram txtShdr{ "assets/text" };
	static VAO txtVAO(txtShdr, UnitBox);
	static BufferObject<TextQuad, GL_ARRAY_BUFFER, GL_STREAM_DRAW> charInstances;

	GetSettings().font.Bind();
	charInstances.Data(FrameVisuals().textInsts);
	txtVAO.BindInstanceData(txtShdr, charInstances);

	txtShdr.use();
	BindPixelUBO();

	txtVAO.Draw();

	//Draw shadows
	glBlendFunc(GL_DST_COLOR, GL_ZERO);

	static ShaderProgram shadowShdr("assets/uishadow");
	static VAO shadowVAO(shadowShdr, UnitBox);
	static BufferObject<Box2z, GL_ARRAY_BUFFER, GL_STREAM_DRAW> shadowInstances;

	shadowInstances.Data(FrameVisuals().shadows);
	shadowVAO.BindInstanceData(shadowShdr, shadowInstances);

	shadowShdr.use();
	GetSettings().screenTex.Bind(0);
	BindPixelUBO();
	shadowVAO.Draw();

	glDisable(GL_BLEND);

	glDisable(GL_DEPTH_TEST);
}

void UI::PushZ(int dz)
{
	auto& stack = FrameVisuals().zStack;
	stack.push_back(stack.back() + dz);
}

void UI::PopZ()
{
	FrameVisuals().zStack.pop_back();
}

int UI::CurZ()
{
	return FrameVisuals().zStack.back();
}

void UI::DrawChar(Eigen::AlignedBox2f pos, Eigen::AlignedBox2f tex)
{
	TextQuad q{ pos, tex, CurZ() };
	FrameVisuals().textInsts.push_back(q);
}

void UI::DrawBox(AlignedBox2i box, Vector4f fill, Vector4f stroke)
{
	FrameVisuals().boxes.push_back({ box, fill, stroke, CurZ() });
}

void UI::DrawBox(AlignedBox2i box)
{
	DrawBox(box, backgroundColor, backgroundColor);
}

void UI::DrawHlBox(AlignedBox2i box)
{
	FrameVisuals().boxes.push_back(
		{ box, Vector4f::Zero(), { .6f, 0.f, 9.f, 1.f }, CurZ() + 1 });
}

void UI::DrawShadow(AlignedBox2i box)
{
	FrameVisuals().shadows.push_back({ box, CurZ() });
}

void UI::DrawQuad(Tex t, AlignedBox2i box, Eigen::AlignedBox2f tex)
{
	FrameVisuals().quads.push_back({ t, { box.cast<float>(), tex, CurZ() } });
}

template<>
const Schema AttribTraits<Box2z>::schema = {
	{ "minBox", GL_INT, true, 0,               { 2, 1 } },
	{ "maxBox", GL_INT, true, 2 * sizeof(int), { 2, 1 } },
	{ "z",      GL_INT, true, 4 * sizeof(int), { 1, 1 } }
};

template<>
const Schema AttribTraits<UIBox>::schema = {
	{ "minBox", GL_INT,   true,  0,               { 2, 1 } },
	{ "maxBox", GL_INT,   true,  2 * sizeof(int), { 2, 1 } },
	{ "fill",   GL_FLOAT, false, 4 * sizeof(int), { 4, 1 } },
	{ "stroke", GL_FLOAT, false, 8 * sizeof(int), { 4, 1 } },
	{ "z",      GL_INT,   true,  12 * sizeof(int), { 1, 1 } }
};

template<>
const Schema AttribTraits<TextQuad>::schema = {
	{ "minBox",    GL_FLOAT, false, 0,                 { 2, 1 } },
	{ "maxBox",    GL_FLOAT, false, 2 * sizeof(float), { 2, 1 } },
	{ "minBoxTex", GL_FLOAT, false, 4 * sizeof(float), { 2, 1 } },
	{ "maxBoxTex", GL_FLOAT, false, 6 * sizeof(float), { 2, 1 } },
	{ "z",         GL_INT,   true,  8 * sizeof(float), { 1, 1 } }
};

static void WinResize(Vector2i sz)
{
	static ShaderProgram txtShdr{ "assets/uibox" };
	static UBO pixelUBO = txtShdr.MakeUBO("Common", "PixelCommon");
	pixelUBO["pixelMat"] = PixelMat(sz);

	auto& s = GetSettings();
	s.screenTex = TypedTex<>(sz);
	s.fbo.AttachTexes({ s.screenTex });
	s.fbo.AttachDepth(RenderBuffer{ GL_DEPTH_COMPONENT, sz });
	s.fbo.CheckStatus();
}

void UI::Init(Window& w)
{
	//init styles
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