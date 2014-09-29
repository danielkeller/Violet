#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "Resource.hpp"

class Tex
{
	struct TexResource;

public:
	static Tex create(std::string path);

	void Bind(GLenum texUnit) const;

	bool operator==(const Tex& other) const
	{
		return false;
	}
	bool operator!=(const Tex& other) const
	{
		return !(*this == other);
	}

private:
	GLuint textureObject;
	std::shared_ptr<TexResource> resource;

	Tex::Tex(std::shared_ptr<TexResource> ptr);
};

#endif