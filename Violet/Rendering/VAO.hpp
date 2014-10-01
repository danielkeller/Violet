#ifndef VAO_HPP
#define VAO_HPP

#include "stdafx.h"
#include "Shader.hpp"
#include "ArrayBuffer.hpp"

#include <vector>
#include <memory>

class VAO
{
	struct VAOResource;

public:
	//initialize to invalid state
	VAO()
		: vertexArrayObject(0)
		, resource(nullptr)
	{}

	void bind() const;
	void draw(GLsizei instances) const;

	bool operator==(const VAO& other) const
	{
		return vertexArrayObject == other.vertexArrayObject;
	}
	bool operator!=(const VAO& other) const
	{
		return !(*this == other);
	}

private:
	GLuint vertexArrayObject;
	GLsizei numVertecies;
	std::shared_ptr<VAOResource> resource;

	VAO(std::shared_ptr<VAOResource> ptr)
		: vertexArrayObject(ptr->vertexArrayObject)
		, numVertecies(ptr->numVertecies)
		, resource(ptr)
	{}

	friend std::tuple<VAO, ShaderProgram> LoadWavefront(std::string filename);

	struct VAOResource : public Resource<VAOResource>
	{
		VAOResource(const VAOResource&) = delete;
		VAOResource(VAOResource&&);

		//clear VAO's resources
		~VAOResource();

		template<class T>
		VAOResource(
			const std::string name,
			const ShaderProgram& program,
			ArrayBuffer<T>&& data,
			const std::vector<GLint>& indices)
			: ResourceTy(name), vertexBuffer()
		{
			glGenVertexArrays(1, &vertexArrayObject);
			glBindVertexArray(vertexArrayObject);

			//gets saved in the VAO
			data.BindToShader(program);
			vertexBuffer = EraseType(std::move(data));

			glGenBuffers(1, &indexBufferObject);

			//GL_ELEMENT_ARRAY_BUFFER binding is part of the current VAO state, therefore we do not unbind it
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObject);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				indices.size()*sizeof(GLint), indices.data(), GL_STATIC_DRAW);

			numVertecies = indices.size();
		}

		//the GL vertex array Render assocated with this Render
		GLuint vertexArrayObject;

		//GL buffer objects for vertex and vertex index data
		GLuint indexBufferObject;
		ArrayBuffer<char> vertexBuffer;

		//how many vertex indices we have
		GLsizei numVertecies;
	};
};

#endif