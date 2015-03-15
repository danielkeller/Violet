#include "stdafx.h"
#include "Material.hpp"
#include "Persist.hpp"

bool Material::operator == (const Material& t) const
{
	return materialProps == t.materialProps && textures == t.textures;
}
bool Material::operator!=(const Material& t) const
{
	return !(*this == t);
}

void Material::use() const
{
	for (GLuint i = 0; i < textures.size(); ++i)
		textures[i].Bind(i);

	materialProps.Bind();
}

std::string Material::Name() const
{
	std::string ret = materialProps.Name();
	for (const auto& tex : textures)
		ret += "#" + tex.Name();
	return ret;
}

void Material::Save(Persist& persist) const
{
	persist.Set<Material>(Name(), materialProps, textures);
}

Material::Material(const UBO& props) : materialProps(props) {}
Material::Material(const UBO& props, const std::vector<Tex>& texs)
	: materialProps(props), textures(texs){}

Material::Material(const std::string&, const UBO& props, const std::vector<Tex>& texs)
	: materialProps(props), textures(texs){}


template<>
const char* PersistSchema<Material>::name = "material";
template<>
Columns PersistSchema<Material>::cols = {"name", "ubo", "texes"};