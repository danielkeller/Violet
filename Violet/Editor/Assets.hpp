#ifndef ASSETS_HPP
#define ASSETS_HPP

#include "Rendering/Texture.hpp"
#include "Rendering/VertexData.hpp"
#include "UI/Elements.hpp"

struct Asset
{
	Asset();
	Asset(std::string name);
	Tex thumb;
	std::string name;
};

class Assets
{
public:
	//true if selection changed
	bool Draw(std::string& cur);
	std::vector<Asset> assets;

	static const int THM_SIZE = 100;
	static const int THM_CHARS = 13;
	static const int THM_SPACE = 8;
	static const int ROWS = 4;
	static const int WIDTH = THM_SIZE*ROWS + THM_SPACE*(ROWS + 1);

private:
	std::vector<UI::Button> buttons;
};

class ObjAssets
{
	Assets a;
public:
	static Tex Thumb(const std::string& path);
	ObjAssets();
	//true if selection changed
	bool Draw(VertexData& cur);
};

#endif