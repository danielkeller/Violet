#include "stdafx.h"
#include "Material.hpp"
#include "File/Persist.hpp"

#include <cstdlib>

Material::Material()
	: id(std::rand())
{}

Material::Material(Id id, Persist& persist)
	: id(id)
{
	std::tie(std::ignore, name, shader, ubo, textures) = persist.Get<Material>(id);
}

Material::Material(const std::string& name, ShaderProgram shader)
	: Material(name, shader, {})
{}
	
Material::Material(const std::string& name, ShaderProgram shader, Tex tex)
	: Material(name, shader, {1, tex})
{}

Material::Material(const std::string& name, ShaderProgram shader, std::vector<Tex> texes)
	: Material(std::rand(), name, shader, shader.MakeUBO("Material", name), texes)
{}

Material::Material(std::int64_t id, const std::string& name, ShaderProgram shader, UBO ubo,
	std::vector<Tex> texes)
	: id(id), name(name), shader(shader)
	, ubo(shader.MakeUBO("Material", name)), textures(texes)
{}

void Material::Save(Persist& persist) const
{
	persist.Set<Material>(id, name, shader, ubo, textures);
}

Material::Id Material::Key() const
{
	return id;
}

void Material::use() const
{
	shader.use();
	for (GLuint i = 0; i < textures.size(); ++i)
		textures[i].Bind(i);
	ubo.Bind();
}

template<>
const char* PersistSchema<Material>::name = "material";
template<>
Columns PersistSchema<Material>::cols = { "id", "name", "shader", "ubo", "texes" };