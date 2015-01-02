#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "Resource.hpp"
#include "MappedFile.hpp"
#include "Lodepng/lodepng.hpp"
#include <iostream>
#include <fstream>

using TexDim = Eigen::Matrix<GLsizei, 2, 1>;

class Tex
{
public:
	Tex(std::string path);

	void Bind(GLuint texUnit) const;
    GLuint Handle() const;
    TexDim Dim() const;

	BASIC_EQUALITY(Tex, textureObject)

protected:
	GLuint textureObject;

	struct TexResource;
	std::shared_ptr<TexResource> resource;
	Tex(std::shared_ptr<TexResource> ptr);
    HAS_HASH
};

MEMBER_HASH(Tex, textureObject)

//TODO: Pixel.hpp
template<class T>
struct PixelTraits
{
    static GLenum internalFormat;
    static GLenum format;
    static GLenum type;
};

using RGBA8Px = Eigen::Matrix<unsigned char, 4, 1>;

template<class Pixel>
class TypedTex : public Tex
{
public:
    TypedTex(TexDim dim);
	TypedTex(std::string path)
        : Tex(path) {}
    HAS_HASH
};

template<class Pixel>
struct std::hash<TypedTex<Pixel>>
{
    size_t operator()(const TypedTex<Pixel>& v) const
        { return std::hash<GLuint>()(v.textureObject);}
};

#include "Texture_detail.hpp"

#endif
