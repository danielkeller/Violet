#ifndef TEXT_HPP
#define TEXT_HPP

#include <memory>

class Font
{
	struct FontResource;
public:
	Font(std::string path);
	std::string Name();

	BASIC_EQUALITY(Font, resource);

	friend void DrawText(const Font &font, const std::string& text, Vector2i pos,
		Vector3f color = {0,0,0}, Vector3f bgColor = {1,1,1});
private:
	std::shared_ptr<FontResource> resource;
};

#endif