#include "stdafx.h"
#include "VAO.hpp"
#include "Shader.hpp"
#include "Rendering/Render.hpp"

#include <fstream>

ConstVAO::VAOResource::VAOResource(VAOResource&& other)
	: ResourceTy(std::move(other))
	, vertexArrayObject(other.vertexArrayObject)
	, indexBufferObject(other.indexBufferObject)
	, vertexBuffer(std::move(other.vertexBuffer))
	, numVertecies(other.numVertecies)
{
	other.indexBufferObject = 0;
	other.vertexArrayObject = 0;
}

ConstVAO::VAOResource::~VAOResource()
{
	//delete VAO and buffers
	glDeleteVertexArrays(1, &vertexArrayObject);
	glDeleteBuffers(1, &indexBufferObject);
}

GLuint ConstVAO::VAOBinding::current;

ConstVAO::VAOBinding::VAOBinding(GLuint next)
	: prev(current)
{
	if (current != next)
	{
		glBindVertexArray(next);
		current = next;
	}
}

ConstVAO::VAOBinding::~VAOBinding()
{
	if (prev != current)
	{
		glBindVertexArray(prev);
		current = prev;
	}
}

void ConstVAO::draw(GLsizei instances) const
{
	auto bound = bind();
	//draw verteces according to the index and position buffer object
	//the final argument to this call is an integer offset, cast to pointer type. don't ask me why.
	glDrawElementsInstanced(mode, numVertecies, GL_UNSIGNED_INT,
		static_cast<GLvoid*>(0), instances);
}

template<>
const Schema ArrayBuffer<VAO::InstanceBuf::value_type>::schema = {
	{ "transform", 4, GL_FLOAT, 0, 4, 4 * sizeof(float) },
};