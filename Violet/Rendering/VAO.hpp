#ifndef VAO_HPP
#define VAO_HPP

#include "stdafx.h"
#include "Shader.hpp"
#include "ArrayBuffer.hpp"

#include <vector>
#include <memory>

class VAO : public Resource<VAO>
{
public:
	//gives us copy semantics, desired object lifetime, equality comparison,
	//and locality of reference
	class Ref
	{
		GLuint vertexArrayObject;
		GLsizei numVertecies;
		std::shared_ptr<VAO> resource;
	public:
		void bind() const;
		void draw(GLsizei instances) const;
		Ref(std::shared_ptr<VAO> ptr)
			: vertexArrayObject(ptr->vertexArrayObject)
			, numVertecies(ptr->numVertecies)
			, resource(ptr)
		{}

		bool operator==(const Ref& other) const
			{ return vertexArrayObject == other.vertexArrayObject; }
		bool operator!=(const Ref& other) const
			{ return !(*this == other); }
	};
	friend Ref;

	friend std::tuple<VAO::Ref, ShaderProgram::Ref> LoadWavefront(std::string filename);

	//VAO is not copyable
	VAO(const VAO&) = delete;
	VAO& operator=(const VAO&) = delete;

	VAO(VAO&&);

	//clear VAO's resources
	~VAO();

private:
	template<class T>
	VAO(const std::string name,
		const ShaderProgram::Ref& program,
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
	ArrayBuffer<void> vertexBuffer;

	//how many vertex indices we have
	GLsizei numVertecies;
};

#endif