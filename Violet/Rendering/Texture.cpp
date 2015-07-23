#include "stdafx.h"

#include "Texture.hpp"

#include "Utils/Profiling.hpp"

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

std::string Tex::Name() const
{
	return resource->Key();
}

Tex::TexResource::TexResource(TexDim dim)
    : ResourceTy("#blankTexture#"), dim(dim)
{
	glGenTextures(1, &textureObject);
}

Tex::TexResource::TexResource(std::string path)
	: ResourceTy(path)
{
	auto p = Profile("texture load");

    const int rgba = 4;
    
	int width, height, components;
	std::unique_ptr<unsigned char, decltype(&::stbi_image_free)> data
		(stbi_load(path.c_str(), &width, &height, &components, rgba), &::stbi_image_free);

	if (!data)
		throw std::runtime_error("Could not open texture '" + path + "', " + stbi_failure_reason());

    //stbi loads the image upside down compared to what opengl wants, so flip it
    for (int row = 0; row < height / 2; ++row)
        for (int col = 0; col < width * rgba; ++col)
            std::iter_swap(&*data + row * width * rgba + col, &*data + (height - row - 1) * width * rgba + col);
    
	dim << width, height;

	glGenTextures(1, &textureObject);
	glBindTexture(GL_TEXTURE_2D, textureObject);
	//Ideally this would be GL_BGRA for performance, but stbi_image doesn't support it
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

//TODO: make this better

template<>
GLenum PixelTraits<std::uint8_t>::internalFormat = GL_R8;
template<>
GLenum PixelTraits<std::uint8_t>::format = GL_RED;
template<>
GLenum PixelTraits<std::uint8_t>::type = GL_UNSIGNED_BYTE;

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

template<>
GLenum PixelTraits<DepthPx>::internalFormat = GL_DEPTH_COMPONENT32F;
template<>
GLenum PixelTraits<DepthPx>::format = GL_DEPTH_COMPONENT;
template<>
GLenum PixelTraits<DepthPx>::type = GL_FLOAT;
