#include "stdafx.h"
#include "Material.hpp"
#include "File/Persist.hpp"

#include <cstdlib>

Material::Material()
	: id(std::rand())
{}


Material::Material(Id id, const Persist& persist)
	: id(id)
{
	UBO::BufferTy buf;
	std::tie(std::ignore, name, shader, buf, textures) = persist.Get<Material>(id);
	ubo = shader.MakeUBO("Material", buf);
}

Material::Material(const std::string& name, ShaderProgram shader)
	: Material(name, shader, {})
{}
	
Material::Material(const std::string& name, ShaderProgram shader, Tex tex)
	: Material(name, shader, {1, tex})
{}

Material::Material(const std::string& name, ShaderProgram shader, std::vector<Tex> texes)
	: id(std::rand()), name(name), shader(shader)
	, ubo(shader.MakeUBO("Material")), textures(texes)
{}

Material::Material(std::int64_t id, const std::string& name, ShaderProgram shader,
	UBO::BufferTy uboBuffer, std::vector<Tex> texes)
	: id(id), name(name), shader(shader)
	, ubo(shader.MakeUBO("Material", uboBuffer)), textures(texes)
{}

void Material::Save(Persist& persist) const
{
	persist.Set<Material>(id, name, shader, ubo.Data(), textures);
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