#include "stdafx.h"

#include "Texture.hpp"

#include "Lodepng/lodepng.hpp"
#include <iostream>
#include <fstream>

struct Tex::TexResource : public Resource<TexResource>
{
	TexResource(TexResource &&other)
		: ResourceTy(std::move(other))
		, textureObject(other.textureObject)
	{
		other.textureObject = 0;
	}

	//free resources associated with this program
	~TexResource();

	//object is not copyable
	TexResource(const TexResource&) = delete;
	TexResource& operator=(const TexResource&) = delete;

	//Construct from given file path
	TexResource(std::string path);

	GLuint textureObject;
};

Tex Tex::create(std::string path)
{
	return TexResource::FindOrMake(path);
}

Tex::Tex(std::shared_ptr<TexResource> ptr)
	: resource(ptr)
	, textureObject(ptr->textureObject)
{}

std::vector<unsigned char> PNGmagic = { 0x89, 0x50, 0x4e, 0x47 };

Tex::TexResource::TexResource(std::string path)
	: ResourceTy(path)
{
	std::basic_ifstream<unsigned char> file(path, std::ios::binary);

	if (!file)
		throw std::runtime_error("Could not open texture '" + path + "'");
	
	std::vector<unsigned char> magic(4);
	file.read(&*magic.begin(), 4);
	file.seekg(0);

	if (!file)
		throw std::runtime_error("Not an image file '" + path + "'");

	std::vector<unsigned char> image;
	unsigned int width, height;

	if (magic == PNGmagic)
	{
		const std::vector<unsigned char> data {
			std::istreambuf_iterator<unsigned char>(file),
			std::istreambuf_iterator<unsigned char>() };

		unsigned int error = lodepng::decode(image, width, height, data);
		if (error)
			throw std::runtime_error(std::string("PNG decoding error: ") + lodepng_error_text(error));
	}
	else
	{
		throw std::runtime_error("Not a recognized image file type '" + path + "'");
	}

	glGenTextures(1, &textureObject);
	glBindTexture(GL_TEXTURE_2D, textureObject);
	//Ideally this would be GL_BGRA for performance, but lodepng doesn't support it
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, image.data());

	//set some reasonable defaults
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
}

Tex::TexResource::~TexResource()
{
	glDeleteTextures(1, &textureObject);
}

void Tex::Bind(GLenum texUnit) const
{
	glActiveTexture(GL_TEXTURE0 + texUnit);
	glBindTexture(GL_TEXTURE_2D, textureObject);
}