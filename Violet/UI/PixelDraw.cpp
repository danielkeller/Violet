#include "stdafx.h"
#include "PixelDraw.hpp"

#include "Rendering/Shader.hpp"
#include "Rendering/VAO.hpp"
#include "Rendering/FBO.hpp"
#include "Window.hpp"
#include "Layout.hpp"
#include "Text.hpp"

using namespace UI;

static const float maxZ = 50.f;

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

std::uint32_t ColorSwizzle(UI::Color color)
{
	std::uint32_t ret = 0;

	ret |= (color & 0xFF000000) >> 24;
	ret |= (color & 0x00FF0000) >> 8;
	ret |= (color & 0x0000FF00) << 8;
	ret |= (color & 0x000000FF) << 24;

	return ret;
}

struct UI::Settings
{
	Settings()
		: screenTex(TexDim{ 0, 0 })
		, pixelUBO({ "assets/uibox" }, "Common")
	{}
	Font font;
	Vector3f textColor;
	TypedTex<DepthPx> screenTex;
	FBO fbo;
	UBO pixelUBO;
	Vector2i winSize;
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

void UI::TextStyle(Font font, Color color)
{
	GetSettings().font = font;
	GetSettings().textColor.x() = float((color >> 24) & 0xFF) / 255.f;
	GetSettings().textColor.y() = float((color >> 16) & 0xFF) / 255.f;
	GetSettings().textColor.z() = float((color >>  8) & 0xFF) / 255.f;
}

struct Box2z
{
	AlignedBox2i box;
	int z;
};

struct UIBox
{
	AlignedBox2i box;
	Color fill, stroke;
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
	std::vector<std::function<void()>> specials;
	std::vector<int> zStack;
};

Visuals& FrameVisuals()
{
	static Visuals vis;
	return vis;
}

struct UIEvents
{
	Events events, dummy;
	bool modalOpen, modalWasOpen, inModal;
};

UIEvents& GetUIEvents()
{
	static UIEvents events;
	return events;
}

Events& UI::FrameEvents()
{
	if (GetUIEvents().modalOpen && !GetUIEvents().inModal)
		return GetUIEvents().dummy;
	else
		return GetUIEvents().events;
}

void UI::PushModal()
{
	GetUIEvents().modalWasOpen = true;
	GetUIEvents().inModal = true;
}

void UI::PopModal()
{
	GetUIEvents().inModal = false;
}

void UI::BeginFrame(Window& w, Events e)
{
	FrameVisuals() = {};
	CurLayout() = LayoutStack(*w.dim);
	GetUIEvents().events = e;
	//set the stuff that matters in dummy
	GetUIEvents().dummy.dimVec = e.dimVec;
	GetUIEvents().dummy.view = e.view;
	GetUIEvents().dummy.simTime = e.simTime;

	//stay in modal mode until a frame passes without a modal events call
	GetUIEvents().modalOpen = GetUIEvents().modalWasOpen;
	GetUIEvents().modalWasOpen = false;
}

LayoutStack& UI::CurLayout()
{
	static LayoutStack layout({ 0, 0 });
	return layout;
}

void UI::SetViewport()
{
	//is this stored per-framebuffer?
	glViewport(0, 0, GetSettings().winSize.x(), GetSettings().winSize.y());
}

void UI::EndFrame()
{
	SetViewport();
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

	{ //get all the depth info
		auto bound = GetSettings().fbo.Bind(GL_FRAMEBUFFER);
		GetSettings().fbo.PreDraw();
		boxVAO.Draw();
	}

	boxVAO.Draw();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
	//blend from constant (text) color to dest color by source color (ie, subpixel alpha)
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

	//Draw specials
	glDisable(GL_BLEND);
	for (auto& fn : FrameVisuals().specials) fn();
	glEnable(GL_BLEND);

	//Draw shadows
	glBlendFunc(GL_DST_COLOR, GL_ZERO); //multiply
	glDepthMask(GL_FALSE); //keep shadows from interfering with each other

	static ShaderProgram shadowShdr("assets/uishadow");
	static VAO shadowVAO(shadowShdr, UnitBox);
	static BufferObject<Box2z, GL_ARRAY_BUFFER, GL_STREAM_DRAW> shadowInstances;

	shadowInstances.Data(FrameVisuals().shadows);
	shadowVAO.BindInstanceData(shadowShdr, shadowInstances);

	shadowShdr.use();
	GetSettings().screenTex.Bind(0);
	BindPixelUBO();
	shadowVAO.Draw();

	glDepthMask(GL_TRUE);
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

void UI::DrawBox(AlignedBox2i box, Color fill, Color stroke)
{
	FrameVisuals().boxes.push_back({ box,
		ColorSwizzle(fill), ColorSwizzle(stroke), CurZ() });
}

void UI::DrawBox(AlignedBox2i box)
{
	DrawBox(box, Colors::bg, Colors::bg);
}

void UI::DrawHlBox(AlignedBox2i box)
{
	FrameVisuals().boxes.push_back({ box, 0, ColorSwizzle(Colors::hilight), CurZ() + 1 });
}

void UI::DrawShadow(AlignedBox2i box)
{
	FrameVisuals().shadows.push_back({ box, CurZ() });
}

void UI::DrawQuad(Tex t, AlignedBox2i box, Eigen::AlignedBox2f tex)
{
	FrameVisuals().quads.push_back({ t, { box.cast<float>(), tex, CurZ() } });
}

void UI::DrawDivider(AlignedBox2i box)
{
	DrawBox({ box.corner(AlignedBox2i::TopLeft),
		box.corner(AlignedBox2i::TopRight) + Vector2i{ 0, 1 } },
		0, Colors::divider);
}

void UI::DrawSpecial(std::function<void()> draw)
{
	FrameVisuals().specials.push_back(draw);
}

template<>
const Schema AttribTraits<Box2z>::schema = {
	AttribProperties{ "minBox", GL_INT, true, 0,               { 2, 1 } },
	AttribProperties{ "maxBox", GL_INT, true, 2 * sizeof(int), { 2, 1 } },
	AttribProperties{ "z",      GL_INT, true, 4 * sizeof(int), { 1, 1 } }
};

template<>
const Schema AttribTraits<UIBox>::schema = {
	AttribProperties{ "minBox", GL_INT,           true,  0,               { 2, 1 } },
	AttribProperties{ "maxBox", GL_INT,           true,  2 * sizeof(int), { 2, 1 } },
	AttribProperties{ "fill",   GL_UNSIGNED_BYTE, false, 4 * sizeof(int), { 4, 1 } },
	AttribProperties{ "stroke", GL_UNSIGNED_BYTE, false, 5 * sizeof(int), { 4, 1 } },
	AttribProperties{ "z",      GL_INT,           true,  6 * sizeof(int), { 1, 1 } }
};

template<>
const Schema AttribTraits<TextQuad>::schema = {
	AttribProperties{ "minBox",    GL_FLOAT, false, 0,                 { 2, 1 } },
	AttribProperties{ "maxBox",    GL_FLOAT, false, 2 * sizeof(float), { 2, 1 } },
	AttribProperties{ "minBoxTex", GL_FLOAT, false, 4 * sizeof(float), { 2, 1 } },
	AttribProperties{ "maxBoxTex", GL_FLOAT, false, 6 * sizeof(float), { 2, 1 } },
	AttribProperties{ "z",         GL_INT,   true,  8 * sizeof(float), { 1, 1 } }
};

static void WinResize(Vector2i sz)
{
	auto& s = GetSettings();
	s.screenTex = TypedTex<DepthPx>(sz);
	s.fbo.AttachDepth(s.screenTex);
	s.fbo.CheckStatus();

	s.pixelUBO["pixelMat"] = PixelMat(sz);

	s.winSize = sz;
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
	GetSettings().pixelUBO.Bind();
}