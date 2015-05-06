#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "Texture.hpp"
#include "Shader.hpp"

class Persist;
struct EmbeddedResourcePersistTag;

struct Material
{
	UBO materialProps;
	std::vector<Tex> textures;

	bool operator==(const Material& t) const;
	bool operator!=(const Material& t) const;

	void use() const;

	std::string Name() const;
	void Save(Persist& persist) const;

	Material() = default;
	Material(UBO props);
	Material(UBO props, Tex tex);
	Material(UBO props, std::vector<Tex> texs);
	Material(const std::string&, UBO props, std::vector<Tex> texs);

	//I guess
	using PersistCategory = EmbeddedResourcePersistTag;

	HAS_HASH
};

MEMBER_HASH(Material, materialProps)

MAKE_PERSIST_TRAITS(Material, std::string, UBO, std::vector<Tex>)

#endif