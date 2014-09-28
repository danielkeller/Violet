#include "stdafx.h"
#include "VAO.hpp"
#include "Shader.hpp"
#include "Rendering/Render.hpp"

#include <fstream>

VAO::VAOResource::VAOResource(VAOResource&& other)
	: ResourceTy(std::move(other))
	, vertexArrayObject(other.vertexArrayObject)
	, indexBufferObject(other.indexBufferObject)
	, vertexBuffer(std::move(other.vertexBuffer))
	, numVertecies(other.numVertecies)
{
	other.indexBufferObject = 0;
	other.vertexArrayObject = 0;
}

VAO::VAOResource::~VAOResource()
{
	//delete VAO and buffers
	glDeleteVertexArrays(1, &vertexArrayObject);
	glDeleteBuffers(1, &indexBufferObject);
}

void VAO::bind() const
{
	//make the object's VAO current. this brings in all the associated data.
	glBindVertexArray(vertexArrayObject);
}

void VAO::draw(GLsizei instances) const
{
	//draw verteces according to the index and position buffer object
	//the final argument to this call is an integer offset, cast to pointer type. don't ask me why.
	glDrawElementsInstanced(GL_TRIANGLES, numVertecies, GL_UNSIGNED_INT,
		static_cast<GLvoid*>(0), instances);
}