#ifndef BOX_HPP
#define BOX_HPP

class Window;

namespace UI
{
	class Font;
	struct Layout;
	struct Settings;
	struct Visuals;

	struct TextQuad
	{
		Eigen::AlignedBox2f pos;
		Eigen::AlignedBox2f tex;
	};

	void DrawChar(TextQuad q);
	void DrawBox(Eigen::AlignedBox2i box);
	void DrawBox(const Layout& l);

	void Init(Window& w);
	void BindPixelUBO();

	void TextStyle(Font font, Vector3f color = { 0, 0, 0 }, Vector3f bgColor = { 1, 1, 1 });

	Font GetFont();

	void EndFrame();
}

#endif