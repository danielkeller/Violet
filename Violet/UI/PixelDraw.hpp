#ifndef BOX_HPP
#define BOX_HPP

class Window;
class Events;

#include "Rendering/Texture.hpp"

namespace UI
{
	class Font;
	struct Layout;
	class LayoutStack;
	struct Settings;
	struct Visuals;

	using AlignedBox2i = Eigen::AlignedBox2i;

	static const Vector4f hlColor = { .6f, 0.f, 9.f, 1.f };

	struct TextQuad
	{
		Eigen::AlignedBox2f pos;
		Eigen::AlignedBox2f tex;
	};

	void DrawChar(TextQuad q);
	void DrawQuad(Tex t, AlignedBox2i box, Eigen::AlignedBox2f tex =
		{ Vector2f{ 0.f, 0.f }, Vector2f{ 1.f, 1.f } });
	void DrawBox(AlignedBox2i box);
	void DrawHlBox(AlignedBox2i box);

	void Init(Window& w);
	void BindPixelUBO();

	void TextStyle(Font font, Vector3f color = { 0, 0, 0 }, Vector3f bgColor = { 1, 1, 1 });

	Font GetFont();

	//fixme: thread safety
	void BeginFrame(Window& w, Events e);
	Events& FrameEvents();
	LayoutStack& CurLayout();
	void EndFrame();
}

#endif