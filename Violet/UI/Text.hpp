#ifndef TEXT_HPP
#define TEXT_HPP

#include <memory>

namespace UI
{
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

		friend void DrawText(const std::string& text, Vector2i pos);
	private:
		std::shared_ptr<FontResource> resource;
	};
}

#endif