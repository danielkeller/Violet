#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "Shader.hpp"
#include "Texture.hpp"

class Persist;
struct EmbeddedResourcePersistTag;

class Material
{
public:
	using Id = std::int64_t;

	Material();
	Material(const Material&) = default;
	Material(Id, const Persist&);
	Material(const std::string& name, ShaderProgram);
	Material(const std::string& name, ShaderProgram, Tex);
	Material(const std::string& name, ShaderProgram, std::vector<Tex>);

	BASIC_EQUALITY(Material, id);

	Id id;
	std::string name;

	ShaderProgram shader;
	UBO ubo;
	std::vector<Tex> textures;

	void use() const;

	void Save(Persist& persist) const;
	Id Key() const;
	using PersistCategory = EmbeddedResourcePersistTag;
	Material(std::int64_t, const std::string&, ShaderProgram, UBO::BufferTy, std::vector<Tex>);
};

MAKE_PERSIST_TRAITS(Material, Material::Id, std::string,
	ShaderProgram, UBO::BufferTy, std::vector<Tex>)

#endif