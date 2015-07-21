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
	TypedTex<DepthPx> screenTex;
	FBO fbo;
	UBO pixelUBO;
	Vector2i winSizePx;
};

Settings& GetSettings()
{
	static Settings settings;
	return settings;
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

struct TexQuad
{
	Eigen::AlignedBox2f pos;
	Eigen::AlignedBox2f tex;
	int z;
};

struct TextureQuad
{
	Tex tex;
	TexQuad quad;
};

struct UI::Visuals
{
	Visuals() : zStack({ 1 }) {}
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
	Events *events, dummy;
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
		return *GetUIEvents().events;
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

void UI::BeginFrame(const Window& w, Events& e)
{
	FrameVisuals() = {};
	CurLayout() = LayoutStack(w.view->ScreenSize());
	GetUIEvents().events = &e;
	//set the stuff that matters in dummy
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
	glViewport(0, 0, GetSettings().winSizePx.x(), GetSettings().winSizePx.y());
}

void UI::EndFrame()
{
    //transfer any undrawn text into quads
    DrawAllText();
    
	SetViewport();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	//Draw boxes

	static ShaderProgram boxShdr("assets/uibox");
	static VAO boxVAO(boxShdr, UnitBox);
	static BufferObject<UIBox, GL_ARRAY_BUFFER, GL_STREAM_DRAW> boxInstances;
    STATIC boxVAO.BindInstanceData(boxShdr, boxInstances);

	boxInstances.Data(FrameVisuals().boxes);
    boxVAO.NumInstances(static_cast<GLsizei>(boxInstances.Size()));

	boxShdr.use();
	BindPixelUBO();

	{ //get all the depth info
		auto bound = GetSettings().fbo.Bind(GL_FRAMEBUFFER);
		GetSettings().fbo.PreDraw();
		boxVAO.Draw();
	}

	boxVAO.Draw();

    //Assume premultipled alpha
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	//Draw textured quads
	static ShaderProgram quadShdr{ "assets/textured_uibox" };
	static VAO quadVAO(quadShdr, UnitBox);
    static BufferObject<TexQuad, GL_ARRAY_BUFFER, GL_STREAM_DRAW> quadInstances(1);
    STATIC quadVAO.BindInstanceData(quadShdr, quadInstances);

	quadShdr.use();
	BindPixelUBO();

	for (const auto& q : FrameVisuals().quads)
	{
		quadInstances.Assign(0, q.quad);
		q.tex.Bind(0);
		quadVAO.Draw();
	}

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

void UI::DrawQuad(Tex t, AlignedBox2i box, Eigen::AlignedBox2f tex, int z)
{
    FrameVisuals().quads.push_back({ t, { box.cast<float>(), tex, z } });
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
const Schema AttribTraits<TexQuad>::schema = {
	AttribProperties{ "minBox",    GL_FLOAT, false, 0,                 { 2, 1 } },
	AttribProperties{ "maxBox",    GL_FLOAT, false, 2 * sizeof(float), { 2, 1 } },
	AttribProperties{ "minBoxTex", GL_FLOAT, false, 4 * sizeof(float), { 2, 1 } },
	AttribProperties{ "maxBoxTex", GL_FLOAT, false, 6 * sizeof(float), { 2, 1 } },
	AttribProperties{ "z",         GL_INT,   true,  8 * sizeof(float), { 1, 1 } }
};

static void WinResize(Viewport view)
{
	auto& s = GetSettings();
    
    if (s.screenTex.Dim() != view.PixelSize())
    {
        s.screenTex = TypedTex<DepthPx>(view.PixelSize());
        s.fbo.AttachDepth(s.screenTex);
        s.fbo.CheckStatus();
    }

	s.pixelUBO["pixelMat"] = PixelMat(view.ScreenSize());
    s.winSizePx = view.PixelSize();
    TextScaling(view.Scaling().x());
}

void UI::Init(Window& w)
{
	w.view += make_magic(accessor<Viewport>(&WinResize));
	WinResize(*w.view);
}

void UI::BindPixelUBO()
{
	GetSettings().pixelUBO.Bind();
}