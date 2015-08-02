#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "Core/Resource.hpp"

struct ResourcePersistTag;

using TexDim = Eigen::Matrix<GLsizei, 2, 1>;
using TexBox = Eigen::AlignedBox<GLsizei, 2>;

class Tex
{
public:
	Tex(std::string path);
    Tex(const char* path) : Tex(std::string{path}) {}

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
    
    //Copy the entirety of 'data' into the portion of 'this' specified by 'box'
    //void SubImage(const Pixel* data, TexBox box);
    
    //Copy the portion of 'data' specified by 'box' into the portion of 'this' specified by 'box'.
    //'data' should be the same size as the texture itself.
    void SubImageOf(const Pixel* data, TexBox box);

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
