#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "Shader.hpp"
#include "Texture.hpp"

class Persist;
struct EmbeddedResourcePersistTag;

class Material
{
	struct Resource;
	std::shared_ptr<Resource> resource;
	Material(std::shared_ptr<Resource>);
    friend std::shared_ptr<Material::Resource> defaultMat();
public:
	using Id = std::int64_t;

    Material(); //default material
	Material(Id, Persist&);
	Material(const Material&) = default;
	Material(const std::string& name, ShaderProgram);
	Material(const std::string& name, ShaderProgram, Tex);
	Material(const std::string& name, ShaderProgram, std::vector<Tex>);

	BASIC_EQUALITY(Material, resource);

	Id GetId() const;
	std::string& Name();
	const std::string& Name() const;
	ShaderProgram& Shader();
	const ShaderProgram& Shader() const;
	UBO& GetUBO();
	std::vector<Tex>& Textures();
	const std::vector<Tex>& Textures() const;

	//Todo: cache friendly
	void use() const;

	void Save(Persist& persist) const;
	Id Key() const;
	using PersistCategory = EmbeddedResourcePersistTag;
};

MAKE_PERSIST_TRAITS(Material, Material::Id, std::string,
	ShaderProgram, UBO::BufferTy, std::vector<Tex>)

#endif