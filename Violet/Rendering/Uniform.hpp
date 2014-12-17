#ifndef UNIFORM_HPP
#define UNIFORM_HPP

#include "Resource.hpp"
#include "BufferObject.hpp"

#include <vector>
#include <memory>

struct Uniforms
{
	Uniforms() = default;
	Uniforms (GLuint program);

	struct Uniform
	{
		std::string name;
		GLenum type;
		GLuint size;
		GLuint offset;
		GLint stride;
		GLint location;

		bool operator<(const Uniform& other) const;
		bool operator==(const Uniform& other) const;
		bool operator!=(const Uniform& other) const
			{ return !(*this == other); }
	};

	struct Block
	{
		std::string name;
		size_t byte_size;
		std::vector<Uniform> uniforms;

		const Uniform& operator[](const std::string& name) const;
		Uniform& operator[](const std::string& name)
		{ return const_cast<Uniform&>(const_cast<const Block&>(*this)[name]); }

		bool operator<(const Block& other) const;
		bool operator==(const Block& other) const;
		bool operator!=(const Block& other) const
			{ return !(*this == other);	}
	};
	std::vector<Block> blocks;

	const Block& operator[](const std::string& name) const;
	Block& operator[](const std::string& name)
	{
		return const_cast<Block&>(const_cast<const Uniforms&>(*this)[name]);
	}
};

//Uniform Buffer Object
class UBO
{
	struct Proxy;
public:
	enum Type
	{
		Common,
		Material
	};

	//It should be possible to call Bind with no ill effects
	UBO()
		: bindProxy(Material)
	{}

	UBO(const Uniforms::Block& block);

	bool operator==(const UBO& other) const
	{ return bindProxy == other.bindProxy; }
	bool operator!=(const UBO& other) const
	{ return !(*this == other);	}

	Proxy operator[](const std::string& name)
		{ return Proxy(*this, name); }

	//synchronize with OpenGL. Note that this stalls anything using this UBO.
	void Sync() const;

	//Associates this UBO with its binding point.
	void Bind() const;

	friend struct Proxy;

private:
	using BufferTy = float; //for alignment
	using BufferObjTy = MutableBufferObject<std::vector<BufferTy>, GL_UNIFORM_BUFFER>;

	struct UBOResource : public Resource<UBOResource, Uniforms::Block>
	{
		BufferObjTy bufferObject;
		//Type allows us to distinguish between UBOs that are shared (ie, camera)
		//with those that are not (ie, material).
		Type type;
		const Uniforms::Block& Block() {return key;}
		UBOResource(const Uniforms::Block& block);
	};

	UBO(std::shared_ptr<UBOResource>);

	BufferObjTy::BufferObjTy::IndexedBindProxy bindProxy;
	std::shared_ptr<UBOResource> resource;

	struct Proxy
	{
		Proxy(UBO& ubo, const std::string& name)
			: ubo(ubo), name(name) {}

		operator Matrix3f() const;
		Proxy& operator=(const Matrix3f&);
		operator Matrix4f() const;
		Proxy& operator=(const Matrix4f&);

	private:
		UBO& ubo;
		const std::string& name;

		template<typename T, GLenum ty>
		T ConvertOpHelper() const;
		template<GLenum ty, typename T>
		UBO::Proxy& AssignOpHelper(const T&);
	};
};

#if 0
#include "stdafx.h"
#include "Shader.hpp"
#include <array>

//get around an inconsistency in the API
inline void CODEGEN_FUNCPTR myUniformMatrix4fv(GLint loc, GLsizei num, const GLfloat* data)
{
	glUniformMatrix4fv(loc, num, GL_FALSE, data);
}

//For non-array uniforms
class UniformSetter
{
	std::array<GLfloat, 16> data;

	void(CODEGEN_FUNCPTR *glFun)(GLint, GLsizei, const GLfloat*);
	GLint location;

#ifndef NDEBUG
	const ShaderProgram::Ref& shader;
#endif

public:
	void Set()
	{
		glFun(location, 1, data.data());
	}

	UniformSetter::UniformSetter(const char* name, const ShaderProgram::Ref& shdr, const Vector3f& val)
		: glFun(glUniform3fv), location(shdr.GetUniformLocation(name))
#ifndef NDEBUG
		, shader(shdr)
#endif
	{
		*this = val;
	}

	UniformSetter& operator= (const Vector3f& val)
	{
		assert(shader.GetUniformType(location) == GL_FLOAT_VEC3);
		std::copy(val.data(), val.data() + val.size(), data.data());
	}

	UniformSetter::UniformSetter(const char* name, const ShaderProgram::Ref& shdr, const Vector4f& val)
		: glFun(glUniform4fv), location(shdr.GetUniformLocation(name))
#ifndef NDEBUG
		, shader(shdr)
#endif
	{
		*this = val;
	}

	UniformSetter& operator= (const Vector4f& val)
	{
		assert(shader.GetUniformType(location) == GL_FLOAT_VEC4);
		std::copy(val.data(), val.data() + val.size(), data.data());
	}

	UniformSetter::UniformSetter(const char* name, const ShaderProgram::Ref& shdr, const Matrix4f& val)
		: glFun(myUniformMatrix4fv), location(shdr.GetUniformLocation(name))
#ifndef NDEBUG
		, shader(shdr)
#endif
	{
		*this = val;
	}

	UniformSetter& operator= (const Matrix4f& val)
	{
		assert(shader.GetUniformType(location) == GL_FLOAT_MAT4);
		std::copy(val.data(), val.data() + val.size(), data.data());
	}
};
#endif

#endif