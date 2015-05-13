#ifndef BOX_HPP
#define BOX_HPP

class Window;
struct Events;

#include "Rendering/Texture.hpp"

namespace UI
{
	class Font;
	struct Layout;
	class LayoutStack;
	struct Settings;
	struct Visuals;

	using AlignedBox2i = Eigen::AlignedBox2i;

	static const Vector4f backgroundColor = { .98f, .98f, .98f, 1 };

	//ortho matrix for pixel drawing
	Matrix4f PixelMat(Vector2i dim);

	//go higher
	void PushZ(int dz = 1);
	//go lower
	void PopZ();
	int CurZ();

	void DrawChar(Eigen::AlignedBox2f pos, Eigen::AlignedBox2f tex);
	void DrawQuad(Tex t, AlignedBox2i box, Eigen::AlignedBox2f tex =
		{ Vector2f{ 0.f, 0.f }, Vector2f{ 1.f, 1.f } });
	void DrawBox(AlignedBox2i box, Vector4f fill, Vector4f stroke);
	void DrawShadow(AlignedBox2i box);
	void DrawHlBox(AlignedBox2i box);
	void DrawBox(AlignedBox2i box);

	void Init(Window& w);
	void BindPixelUBO();

	void TextStyle(Font font, Vector3f color = { .1f, .1f, .1f });

	Font GetFont();

	//fixme: thread safety
	void BeginFrame(Window& w, Events e);
	Events& FrameEvents();
	LayoutStack& CurLayout();
	void EndFrame();
}

#endif