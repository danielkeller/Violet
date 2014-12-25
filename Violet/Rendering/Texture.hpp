#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "Resource.hpp"

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
};

//integer texture for screen picking
class IntTex
{
public:
    IntTex(GLsizei width, GLsizei height);
    IntTex(const IntTex&) = delete;
    IntTex(IntTex&&);
    ~IntTex();

	void Bind(GLuint texUnit) const;
    GLuint Handle() const;
private:
	GLuint textureObject;
};

#endif
