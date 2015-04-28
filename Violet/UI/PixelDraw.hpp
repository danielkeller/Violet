#ifndef BOX_HPP
#define BOX_HPP

class Window;
class Events;

namespace UI
{
	class Font;
	struct Layout;
	class LayoutStack;
	struct Settings;
	struct Visuals;

	using AlignedBox2i = Eigen::AlignedBox2i;

	struct TextQuad
	{
		Eigen::AlignedBox2f pos;
		Eigen::AlignedBox2f tex;
	};

	void DrawChar(TextQuad q);
	void DrawBox(AlignedBox2i box);
	void DrawBox(const Layout& l);

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