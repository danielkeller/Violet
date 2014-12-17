#ifndef SHADER_H
#define SHADER_H

#include "Resource.hpp"
#include "Uniform.hpp"

#include <istream>
#include <memory>

class ShaderProgram
{
	struct ShaderResource;

public:
	ShaderProgram(std::string path);

	//Enable this program for rendering
	void use() const;

	//get an attribute
	GLint GetAttribLocation(const char* name) const;
	GLenum GetAttribType(GLint loc) const;

	BASIC_EQUALITY(ShaderProgram, program)

	std::string Name() const;
	Uniforms& Uniforms() const;
	UBO GetUBO(const std::string& name) const;
	void TextureOrder(const std::vector<std::string>& order);

private:
	ShaderProgram(std::shared_ptr<ShaderResource> ptr);
	struct ShaderResource;

	GLuint program;
	std::shared_ptr<ShaderResource> resource;
};

#endif
