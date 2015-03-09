#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "Texture.hpp"

struct Material
{
	UBO materialProps;
	std::vector<Tex> textures;

	bool operator==(const Material& t) const
	{
		return materialProps == t.materialProps && textures == t.textures;
	}
	bool operator!=(const Material& t) const
	{
		return !(*this == t);
	}

	void use() const
	{
		for (GLuint i = 0; i < textures.size(); ++i)
			textures[i].Bind(i);

		materialProps.Bind();
	}

	Material() = default;
	Material(const UBO& props) : materialProps(props) {}
	Material(const UBO& props, const std::vector<Tex>& texs)
		: materialProps(props), textures(texs){}

	HAS_HASH
};

MEMBER_HASH(Material, materialProps)

#endif