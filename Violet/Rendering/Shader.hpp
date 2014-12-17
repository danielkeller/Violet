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
	static ShaderProgram create(std::string path)
	{
		return ShaderResource::FindOrMake(path);
	}

	//Enable this program for rendering
	void use() const;

	//get an attribute
	GLint GetAttribLocation(const char* name) const;
	GLenum GetAttribType(GLint loc) const;

	bool operator==(const ShaderProgram& other) const
	{
		return program == other.program;
	}
	bool operator!=(const ShaderProgram& other) const
	{
		return !(*this == other);
	}

	std::string Name() const { return resource->Name(); }
	Uniforms& Uniforms() const { return resource->uniforms; }
	UBO GetUBO(const std::string& name) const;
	void TextureOrder(const std::vector<std::string>& order);

private:
	//initialize to invalid state
	ShaderProgram()
		: program(0)
		, resource(nullptr)
	{}

	ShaderProgram(std::shared_ptr<ShaderResource> ptr)
		: program(ptr->program)
		, resource(std::move(ptr))
	{}

	GLuint program;
	std::shared_ptr<ShaderResource> resource;

	struct ShaderResource : public Resource<ShaderResource>
	{
		ShaderResource(ShaderResource &&other)
			: ResourceTy(std::move(other)), program(other.program)
			, uniforms(std::move(other.uniforms))
		{
			other.program = 0;
		}

		//free resources associated with this program
		~ShaderResource();

		//object is not copyable
		ShaderResource(const ShaderResource&) = delete;
		ShaderResource& operator=(const ShaderResource&) = delete;

		//Construct from given vertex and fragment files
		ShaderResource(std::string path);

		std::string Name() const { return key; }

		//the actual GL program reference
		GLuint program;
		void init(std::istream &vert, std::istream &frag);

		::Uniforms uniforms;
	};
};

#endif
