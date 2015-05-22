#include "stdafx.h"
#include "VAO.hpp"
#include "Shader.hpp"
#include <iostream>

void VAO::BindArrayBufToShader(const ShaderProgram& program, const Schema& schema,
	GLsizei stride, GLsizei offset, bool instanced, bool warnings)
{
	auto bound = Bind();
	for (const auto& props : schema)
	{
		//enable generic attribute array vertAttrib in the current vertex array object (VAO)
		GLint vertAttrib = program.GetAttribLocation(props.name);
		if (vertAttrib == -1)
		{
			if (warnings)
				std::cerr << "Warning: Vertex attribute '" << props.name
					<< "' is not defined or active in '" << program.Name() << "'\n";
			continue;
		}

		//GL makes you specify matrices in this goofball way
		for (int column = 0; column < props.dims[1]; ++column)
		{
			glEnableVertexAttribArray(vertAttrib + column);

			//associate the buffer data bound to GL_ARRAY_BUFFER with the attribute in index 0
			//the final argument to this call is an integer offset, cast to pointer type. don't ask me why.
            if (props.integer)
                glVertexAttribIPointer(
                    vertAttrib + column, props.dims[0], props.glType, stride,
                    static_cast<const char*>(nullptr)
                    + offset*stride + props.offset + column*props.matrixStride);
            else
                glVertexAttribPointer(
                    vertAttrib + column, props.dims[0], props.glType, GL_TRUE, stride,
                    static_cast<const char*>(nullptr)
				    + offset*stride + props.offset + column*props.matrixStride);

			if (instanced)
				glVertexAttribDivisor(vertAttrib + column, 1);
		}
	}
}

VAO::VAO(const ShaderProgram& program, const VertexData& vertdata)
	: vertexData(vertdata)
	, numVertecies(vertdata.resource->numVertecies)
	, numInstances(1)
	, mode(vertdata.resource->mode)
{
	glGenVertexArrays(1, &vertexArrayObject);
	auto bound = Bind();
	auto& res = *vertexData.resource;
	res.indexBuffer.Bind();
	res.vertexBuffer.Bind();
	BindArrayBufToShader(program, res.vertexBufferSchema, res.vertexBufferStride);
}

VAO::VAO(VAO&& other)
	: vertexArrayObject(other.vertexArrayObject)
	, vertexData(std::move(other.vertexData))
	, numVertecies(other.numVertecies)
	, numInstances(other.numInstances)
	, mode(other.mode)
{
	other.vertexArrayObject = 0;
}

VAO::~VAO()
{
	glDeleteVertexArrays(1, &vertexArrayObject);
}

VAO& VAO::operator=(VAO v)
{
	swap(*this, v);
	return *this;
}

void swap(VAO& l, VAO& r)
{
	swap(l.vertexArrayObject, r.vertexArrayObject);
	swap(l.vertexData, r.vertexData);
	swap(l.numVertecies, r.numVertecies);
	swap(l.numInstances, r.numInstances);
	swap(l.mode, r.mode);
}

GLuint VAO::Binding::current = 0;

VAO::Binding::Binding(GLuint next)
	: prev(current)
{
	if (current != next)
	{
		glBindVertexArray(next);
		current = next;
	}
}

VAO::Binding::~Binding()
{
	if (prev != current)
	{
		glBindVertexArray(prev);
		current = prev;
	}
}

void VAO::Draw() const
{
	auto bound = Bind();
	//draw verteces according to the index and position buffer object
	//the final argument to this call is an integer offset, cast to pointer type. don't ask me why.
	glDrawElementsInstanced(mode, numVertecies, GL_UNSIGNED_INT,
		static_cast<GLvoid*>(0), numInstances);
}
