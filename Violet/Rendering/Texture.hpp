#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "Resource.hpp"

using TexDim = Eigen::Matrix<GLsizei, 2, 1>;

class Tex
{
	struct TexResource;

public:
	Tex(std::string path);

	void Bind(GLuint texUnit) const;
	BASIC_EQUALITY(Tex, textureObject)

private:
	GLuint textureObject;
	std::shared_ptr<TexResource> resource;
	Tex(std::shared_ptr<TexResource> ptr);
    HAS_HASH
};

MEMBER_HASH(Tex, textureObject)

//TODO:
//template<class Pixel>
//PixelTraits<Pixel>::format etc
//integer texture for screen picking
class IntTex
{
public:
    IntTex(TexDim dim);
    IntTex(const IntTex&) = delete;
    IntTex(IntTex&&);
    ~IntTex();

	void Bind(GLuint texUnit) const;
    GLuint Handle() const;
    TexDim Dim() const;
private:
	GLuint textureObject;
    TexDim dim;
};

#endif
