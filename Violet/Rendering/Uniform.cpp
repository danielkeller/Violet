#include "stdafx.h"
#include "Uniform.hpp"
#include <numeric>
#include <iostream>

std::vector<Uniforms::Block> DoQuery(GLuint program)
{
	GLint activeUniforms;
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &activeUniforms);
	
	GLint activeUniformBlocks;
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &activeUniformBlocks);
	std::vector<Uniforms::Block> ret(activeUniformBlocks+1);
	GLint outParam;

	for (GLuint uniformBlockIndex = 0; uniformBlockIndex < (GLuint)activeUniformBlocks; ++uniformBlockIndex)
	{
		glGetActiveUniformBlockiv(program, uniformBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &outParam);
		ret[uniformBlockIndex].byte_size = outParam;
		glGetActiveUniformBlockiv(program, uniformBlockIndex, GL_UNIFORM_BLOCK_NAME_LENGTH, &outParam);
		std::vector<char> name(outParam);
		glGetActiveUniformBlockName(program, uniformBlockIndex, outParam, NULL, name.data());
		ret[uniformBlockIndex].name.assign(name.begin(), name.end()-1); //remove null
	}

	for (GLuint uniformIndex = 0; uniformIndex < (GLuint)activeUniforms; ++uniformIndex)
	{
		Uniforms::Uniform unif;
		glGetActiveUniformsiv(program, 1, &uniformIndex, GL_UNIFORM_TYPE, &outParam);
		unif.type = static_cast<GLenum>(outParam);
		glGetActiveUniformsiv(program, 1, &uniformIndex, GL_UNIFORM_SIZE, &outParam);
		unif.size = outParam;
		glGetActiveUniformsiv(program, 1, &uniformIndex, GL_UNIFORM_OFFSET, &outParam);
		unif.offset = outParam;
		glGetActiveUniformsiv(program, 1, &uniformIndex, GL_UNIFORM_ARRAY_STRIDE, &outParam);
		unif.stride = outParam;
		//Hopefully the matrix stride isn't a big deal...

		glGetActiveUniformsiv(program, 1, &uniformIndex, GL_UNIFORM_NAME_LENGTH, &outParam);
		std::vector<char> name(outParam);
		glGetActiveUniformName(program, uniformIndex, outParam, NULL, name.data());
		unif.name.assign(name.begin(), name.end()-1); //remove null

		unif.location = glGetUniformLocation(program, name.data());

		glGetActiveUniformsiv(program, 1, &uniformIndex, GL_UNIFORM_BLOCK_INDEX, &outParam);

		if (outParam == -1) //last block is default block
			ret[activeUniformBlocks].uniforms.push_back(std::move(unif));
		else
			ret[outParam].uniforms.push_back(std::move(unif));
	}

	return ret;
}

Uniforms::Uniforms(GLuint program)
	: blocks(DoQuery(program))
{}

bool Uniforms::Uniform::operator==(const Uniform& o) const
{
	return std::tie(name, type, size, offset, stride)
		== std::tie(o.name, o.type, o.size, o.offset, o.stride);
}

bool Uniforms::Uniform::operator<(const Uniform& o) const
{
	return std::tie(name, type, size, offset, stride)
		< std::tie(o.name, o.type, o.size, o.offset, o.stride);
}

bool Uniforms::Block::operator==(const Block& o) const
{
	return std::tie(byte_size, name, uniforms) == std::tie(o.byte_size, o.name, o.uniforms);
}

bool Uniforms::Block::operator<(const Block& o) const
{
	return std::tie(byte_size, name, uniforms) < std::tie(o.byte_size, o.name, o.uniforms);
}

const Uniforms::Uniform& Uniforms::Block::operator[](const std::string& n) const
{
	auto it = std::find_if(uniforms.begin(), uniforms.end(), [&n](const Uniforms::Uniform& u)
	{return u.name == n; });
	assert(it != uniforms.end());
	return *it;
}

const Uniforms::Block& Uniforms::operator[](const std::string& n) const
{
	auto it = std::find_if(blocks.begin(), blocks.end(), [&n](const Uniforms::Block& u)
	{return u.name == n; });
	assert(it != blocks.end());
	return *it;
}

UBO UBO::Create(const Uniforms::Block& block)
{
	return UBO(UBOResource::FindOrMake(block));
}

UBO::UBOResource::UBOResource(const Uniforms::Block& block)
	: Resource(block), bufferObject(block.byte_size / sizeof(BufferTy))
	, type(block.name == "Common" ? UBO::Common : UBO::Material)
{}

UBO::UBO(std::shared_ptr<UBOResource> r)
	: bindProxy(r->bufferObject.BufferObj().GetIndexedBindProxy(r->type))
	, resource(r)
{}

void UBO::Sync() const
{
	resource->bufferObject.Sync();
}

void UBO::Bind() const
{
	bindProxy.Bind();
}

template<typename T, GLenum ty>
T UBO::Proxy::ConvertOpHelper() const
{
	const Uniforms::Uniform& unif = ubo.resource->Block()[name];
	assert(unif.type == ty);
	T ret;
	const typename T::Scalar* store = ubo.resource->bufferObject.Container().data();
	std::copy(store + unif.offset,
		store + unif.offset + ret.size(),
		ret.data());
	return ret;
}

template<GLenum ty, typename T>
UBO::Proxy& UBO::Proxy::AssignOpHelper(const T& val)
{
	const Uniforms::Uniform& unif = ubo.resource->Block()[name];
	assert(unif.type == ty);
	typename T::Scalar* store = ubo.resource->bufferObject.Container().data();
	std::copy(val.data(),
		val.data() + val.size(),
		store + unif.offset);
	return *this;
}

UBO::Proxy::operator Matrix3f() const
{
	return ConvertOpHelper<Matrix3f, GL_FLOAT_MAT3>();
}

UBO::Proxy& UBO::Proxy::operator=(const Matrix3f& v)
{
	return AssignOpHelper<GL_FLOAT_MAT3>(v);
}

UBO::Proxy::operator Matrix4f() const
{
	return ConvertOpHelper<Matrix4f, GL_FLOAT_MAT4>();
}

UBO::Proxy& UBO::Proxy::operator=(const Matrix4f& v)
{
	return AssignOpHelper<GL_FLOAT_MAT4>(v);
}
