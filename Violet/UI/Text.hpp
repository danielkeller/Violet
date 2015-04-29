#ifndef TEXT_HPP
#define TEXT_HPP

#include <memory>

namespace UI
{
	using AlignedBox2i = Eigen::AlignedBox2i;

	class Font
	{
		struct FontResource;
	public:
		//Default font
		Font();
		Font(std::string path);
		std::string Name();

		void Bind();

		BASIC_EQUALITY(Font, resource);

		friend Vector2i TextDim(const std::string& text);
		friend void DrawText(const std::string& text, Vector2i pos);
		//centers the text in the box
		friend void DrawText(const std::string& text, AlignedBox2i container);
	private:
		std::shared_ptr<FontResource> resource;
	};
}

#endif