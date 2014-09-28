#ifndef SHADER_H
#define SHADER_H

#include "stdafx.h"
#include "Resource.hpp"
#include "Uniform.hpp"

#include <istream>
#include <memory>

class ShaderProgram : public Resource<ShaderProgram>
{
public:
	class Ref
	{
		GLuint program;
		std::shared_ptr<ShaderProgram> resource;
	public:
		Ref(std::shared_ptr<ShaderProgram> ptr)
			: program(ptr->program)
			, resource(std::move(ptr))
		{}

		//Enable this program for rendering
		void use() const;

		//get an attribute
		GLint GetAttribLocation(const char* name) const;
		GLenum GetAttribType(GLint loc) const;

		bool operator==(const Ref& other) const
			{ return program == other.program; }
		bool operator!=(const Ref& other) const
			{ return !(*this == other); }

		std::string Name() const { return resource->key; }
		Uniforms& Uniforms() const { return resource->uniforms; }
		UBO GetUBO(const std::string& name) const;
	};
	friend Ref;

	static Ref create(std::string path)
	{
		return ShaderProgram::FindOrMake(path);
	}

	ShaderProgram(ShaderProgram &&other)
		: ResourceTy(other), program(other.program)
		, uniforms(std::move(other.uniforms))
	{
		other.program = 0;
	}

    //free resources associated with this program
	~ShaderProgram();

	//object is not copyable
	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = delete;

	//Construct from given vertex and fragment files
	ShaderProgram(std::string path);

private:
    //the actual GL program reference
    GLuint program;
    void init(std::istream &vert, std::istream &frag);

	Uniforms uniforms;
};

#endif