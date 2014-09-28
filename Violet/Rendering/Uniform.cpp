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
	std::vector<Uniforms::Block> ret(activeUniformBlocks);
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
		unif.type = outParam;
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

		glGetActiveUniformsiv(program, 1, &uniformIndex, GL_UNIFORM_BLOCK_INDEX, &outParam);
		if (outParam == -1)
			std::cerr << "Warning: Uniform '" << unif.name
				<< "' not in a block; this is not supported\n";
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

UBO UBO::Create(const Uniforms::Block& block, Type ty)
{
	return UBO(UBOResource::FindOrMake(block), ty);
}

UBO::UBOResource::UBOResource(const Uniforms::Block& block)
	: Resource(block), storage(block.byte_size)
{
	//generate a new buffer object for bufferObject
	glGenBuffers(1, &bufferObject);

	//bind the newly-created buffer object bufferObject as the current GL_UNIFORM_BUFFER
	glBindBuffer(GL_UNIFORM_BUFFER, bufferObject);
	//allocate the buffer, but don't upload anything
	glBufferData(GL_UNIFORM_BUFFER, block.byte_size, NULL, GL_DYNAMIC_DRAW);
}

UBO::UBOResource::~UBOResource()
{
	glDeleteBuffers(1, &bufferObject);
}

UBO::UBO(std::shared_ptr<UBOResource> r, Type ty)
	: type(ty), bufferObject(r->bufferObject), resource(r)
{}

void UBO::Sync()
{
	glBindBuffer(GL_UNIFORM_BUFFER, bufferObject);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, resource->storage.size(), resource->storage.data());
}

void UBO::Bind()
{
	//Type allows us to distinguish between UBOs that are shared (ie, camera) with those that
	//are not (ie, material).
	glBindBufferBase(GL_UNIFORM_BUFFER, type, bufferObject);
}

template<typename T, GLenum ty>
T UBO::Proxy::ConvertOpHelper() const
{
	const Uniforms::Uniform& unif = ubo.resource->Block()[name];
	assert(unif.type == ty);
	T ret;
	const T::Scalar* store = ubo.resource->storage.data();
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
	T::Scalar* store = ubo.resource->storage.data();
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