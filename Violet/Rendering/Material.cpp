#include "stdafx.h"
#include "Material.hpp"
#include "File/Persist.hpp"

#include <cstdlib>

#include <iostream>

struct Material::Resource : public ::Resource<Resource, Id>
{
	Resource(Id id, std::string name, ShaderProgram shader, UBO ubo, std::vector<Tex> textures)
	: ResourceTy(id), name(name), shader(shader), ubo(std::move(ubo)), textures(textures)
	{}

	Resource(Id id, std::string name, ShaderProgram shader,
		UBO::BufferTy uboData, std::vector<Tex> textures)
		: Resource(id, name, shader, UBO(shader, "Material", uboData), textures)
	{}

	std::string name;
	ShaderProgram shader;
	UBO ubo;
	std::vector<Tex> textures;
};

Material::Material(Id id, Persist& persist)
	: Material(Resource::FindResource(id) ? Resource::FindResource(id)
		: invoke(Resource::MakeShared<Material::Id, std::string,
			ShaderProgram, UBO::BufferTy, std::vector<Tex>>,
			persist.Get<Material>(id)))
{}


Material::Material(const std::string& name, ShaderProgram shader)
	: Material(name, shader, {})
{}
	
Material::Material(const std::string& name, ShaderProgram shader, Tex tex)
	: Material(name, shader, {1, tex})
{}

Material::Material(const std::string& name, ShaderProgram shader, std::vector<Tex> texes)
	: resource(Resource::MakeShared(
		std::rand(), name, shader, UBO{ shader, "Material" }, texes))
{}

Material::Material(std::shared_ptr<Resource> resource)
	: resource(resource)
{}

void Material::Save(Persist& persist) const
{
	persist.Set<Material>(resource->Key(), resource->name, resource->shader,
		resource->ubo.Data(), resource->textures);
}

Material::Id Material::Key() const
{
	return resource->Key();
}

Material::Id Material::GetId() const { return resource->Key(); }
std::string& Material::Name() { return resource->name; }
const std::string& Material::Name() const { return resource->name; }
ShaderProgram& Material::Shader() { return resource->shader; }
const ShaderProgram& Material::Shader() const { return resource->shader; }
UBO& Material::GetUBO() { return resource->ubo; }
std::vector<Tex>& Material::Textures() { return resource->textures; }
const std::vector<Tex>& Material::Textures() const { return resource->textures; }

void Material::use() const
{
	resource->shader.use();
	for (GLuint i = 0; i < resource->textures.size(); ++i)
		resource->textures[i].Bind(i);
	resource->ubo.Bind();
}

template<>
const char* PersistSchema<Material>::name = "material";
template<>
Columns PersistSchema<Material>::cols = { "id", "name", "shader", "ubo", "texes" };