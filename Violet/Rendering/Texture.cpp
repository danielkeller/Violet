#include "stdafx.h"

#include "Texture.hpp"

#include "MappedFile.hpp"
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

Tex::Tex(std::string path)
	: Tex(TexResource::FindOrMake(path))
{}

Tex::Tex(std::shared_ptr<TexResource> ptr)
	: textureObject(ptr->textureObject)
	, resource(ptr)
{}

std::vector<unsigned char> PNGmagic = { 0x89, 0x50, 0x4e, 0x47 };

Tex::TexResource::TexResource(std::string path)
	: ResourceTy(path)
{
    MappedFile file;
    file.Throws(true);
    file.Open(path);

	if (!file)
		throw std::runtime_error("Could not open texture '" + path + "'");

	std::vector<unsigned char> magic(file.Data<unsigned char>(), file.Data<unsigned char>() + 4);

	std::vector<unsigned char> image;
	unsigned int width, height;

	if (magic == PNGmagic)
	{
		unsigned int error = lodepng::decode(image, width, height, file.Data<unsigned char>(), file.Size());
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
	glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_RGBA), width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, image.data());

	//set some reasonable defaults
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR_MIPMAP_LINEAR));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_MIRRORED_REPEAT));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_MIRRORED_REPEAT));

    file.Close();
}

Tex::TexResource::~TexResource()
{
	glDeleteTextures(1, &textureObject);
}

void Tex::Bind(GLuint texUnit) const
{
	glActiveTexture(static_cast<GLenum>(static_cast<GLuint>(GL_TEXTURE0) + texUnit));
	glBindTexture(GL_TEXTURE_2D, textureObject);
}

IntTex::IntTex(GLsizei width, GLsizei height)
{
	glGenTextures(1, &textureObject);
	glBindTexture(GL_TEXTURE_2D, textureObject);
	//Ideally this would be GL_BGRA for performance, but lodepng doesn't support it
	glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_R32UI), width, height, 0,
		GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
}

IntTex::IntTex(IntTex&& other)
    : textureObject(other.textureObject)
{
    other.textureObject = 0;
}

IntTex::~IntTex()
{
	glDeleteTextures(1, &textureObject);
}

GLuint IntTex::Handle() const
{
    return textureObject;
}

void IntTex::Bind(GLuint texUnit) const
{
	glActiveTexture(static_cast<GLenum>(static_cast<GLuint>(GL_TEXTURE0) + texUnit));
	glBindTexture(GL_TEXTURE_2D, textureObject);
}
