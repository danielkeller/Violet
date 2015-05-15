#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "Core/Resource.hpp"

struct ResourcePersistTag;

using TexDim = Eigen::Matrix<GLsizei, 2, 1>;

class Tex
{
public:
	Tex(std::string path);

	void Bind(GLuint texUnit) const;
    GLuint Handle() const;
    TexDim Dim() const;

	BASIC_EQUALITY(Tex, textureObject);

	std::string Name() const;

	using PersistCategory = ResourcePersistTag;

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

struct DepthPx
{
	float depth;
};

template<class Pixel = RGBA8Px>
class TypedTex : public Tex
{
public:
    TypedTex(TexDim dim);
	TypedTex(std::string path)
        : Tex(path) {}

	//must be right amout of data
	void Image(const Pixel* data);

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
