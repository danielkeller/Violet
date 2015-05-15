#ifndef ASSETS_HPP
#define ASSETS_HPP

#include "Rendering/Texture.hpp"
#include "Rendering/Material.hpp"
#include "UI/Elements.hpp"

class Material;
class VertexData;

template<typename Key>
struct Asset
{
	Asset();
	Asset(std::string name, Key key);
	Tex thumb;
	std::string name;
	Key key;
};

template<typename Key>
class Assets
{
public:
	//true if selection changed
	bool Draw(Key& cur);
	std::vector<Asset<Key>> assets;

private:
	std::vector<UI::Button> buttons;
};

//FIXME: Both of these should rescan when sensible

class ObjAssets
{
	Assets<std::string> a;
public:
	static Tex Thumb(const std::string& path);
	ObjAssets();
	//true if selection changed
	bool Draw(VertexData& cur);
};

class MaterialAssets
{
	Assets<Material::Id> a;
public:
	static Tex Thumb(const Material& mat);
	MaterialAssets(Persist&);
	//true if selection changed
	bool Draw(Material& cur, Persist&);
};

namespace Asset_detail
{
	static const int THM_SIZE = 100;
	static const int THM_CHARS = 13;
	static const int THM_SPACE = 8;
	static const int ROWS = 4;
	static const int WIDTH = THM_SIZE*ROWS + THM_SPACE*(ROWS + 1);
}

#endif