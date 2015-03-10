#include "stdafx.h"

#include "Texture.hpp"

#include "MappedFile.hpp"
#include "Profiling.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG //enable additional ones as needed
#define STBI_FAILURE_USERMSG
#include "stb/stb_image.h"

#include <cstdint>

Tex::Tex(std::string path)
	: Tex(TexResource::FindOrMake(path))
{}

Tex::Tex(std::shared_ptr<TexResource> ptr)
	: textureObject(ptr->textureObject)
	, resource(ptr)
{}

std::vector<unsigned char> PNGmagic = { 0x89, 0x50, 0x4e, 0x47 };

Tex::TexResource::TexResource(TexDim dim)
    : ResourceTy("#blankTexture#"), dim(dim)
{
	glGenTextures(1, &textureObject);
}

Tex::TexResource::TexResource(std::string path)
	: ResourceTy(path)
{
	auto p = Profile::Profile("texture load");

	int width, height, components;
	std::unique_ptr<unsigned char, decltype(&::stbi_image_free)> data
		(stbi_load(path.c_str(), &width, &height, &components, 0), &::stbi_image_free);

	if (!data)
		throw std::runtime_error("Could not open texture '" + path + "', " + stbi_failure_reason());

	dim << width, height;

	glGenTextures(1, &textureObject);
	glBindTexture(GL_TEXTURE_2D, textureObject);
	//Ideally this would be GL_BGRA for performance, but lodepng doesn't support it
	glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_RGBA), dim.x(), dim.y(), 0,
		GL_RGBA, GL_UNSIGNED_BYTE, data.get());

	//set some reasonable defaults
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR_MIPMAP_LINEAR));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_MIRRORED_REPEAT));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_MIRRORED_REPEAT));
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

TexDim Tex::Dim() const
{
    return resource->dim;
}

GLuint Tex::Handle() const
{
    return textureObject;
}

template<>
GLenum PixelTraits<std::uint32_t>::internalFormat = GL_R32UI;
template<>
GLenum PixelTraits<std::uint32_t>::format = GL_RED_INTEGER;
template<>
GLenum PixelTraits<std::uint32_t>::type = GL_UNSIGNED_INT;

template<>
GLenum PixelTraits<RGBA8Px>::internalFormat = GL_RGBA8;
template<>
GLenum PixelTraits<RGBA8Px>::format = GL_RGBA;
template<>
GLenum PixelTraits<RGBA8Px>::type = GL_UNSIGNED_BYTE;
