#ifndef SHADER_H
#define SHADER_H

#include "Core/Resource.hpp"
#include "BufferObject.hpp"

#include <cstdint>

struct UniformType
{
	UniformType() = default;
	UniformType(GLenum type);
	GLenum scalar;
	GLuint rows, cols;
};

inline GLenum UniformScalar(float) { return GL_FLOAT; }
inline GLenum UniformScalar(int) { return GL_INT; }
inline GLenum UniformScalar(unsigned int) { return GL_UNSIGNED_INT; }

struct Uniform
{
	std::string name;
	UniformType type;
	GLuint size;
	GLuint offset;
	GLint stride, location;

	MEMBER_EQUALITY(std::string, name)
};

//Uniform Buffer Object
class UBO
{
	struct Proxy;
	friend struct Proxy;
	friend class ShaderProgram;
	enum Type
	{
		Common,
		Material
	};

	//typename std::aligned_storage<sizeof(char), alignof(Matrix4f)>::type;
	using BufferElemTy = char;

public:
	//It should be possible to call Bind with no ill effects
	UBO();

	BASIC_EQUALITY(UBO, bindProxy)

	Proxy operator[](const std::string& name);

	//Associates this UBO with its binding point.
	void Bind() const;

	using BufferTy = std::vector<BufferElemTy>;
	const BufferTy& Data() const;
	std::vector<Uniform> Uniforms() const;

	//Only needed when editing the data through []::Map()
	void Sync();

private:
	using BufferObjTy = BufferObject<BufferElemTy, GL_UNIFORM_BUFFER, GL_STREAM_DRAW>;
	struct Impl;
	BufferObjTy::IndexedBindProxy bindProxy;
	std::shared_ptr<Impl> impl;

	UBO(std::shared_ptr<Impl>);
	HAS_HASH
};

struct ResourcePersistTag;
struct EmbeddedResourcePersistTag;
class Persist;

//FIXME: static instances of this class can cause crashes at exit if the
//destructor happens to be called after the gl module is unloaded and
//glDeleteProgram hasn't been called (ie, loaded) yet. I think this only
//happens in the case of unusual termination though
class ShaderProgram
{
	struct ShaderResource;

public:
    ShaderProgram() : ShaderProgram("assets/blank") {}
	ShaderProgram(std::string path);
	std::string Name() const;
	BASIC_EQUALITY(ShaderProgram, program)

	//Enable this program for rendering
	void use() const;

	//get an attribute
	GLint GetAttribLocation(const std::string& name) const;
	GLenum GetAttribType(GLint loc) const;

	UBO MakeUBO(const std::string& block) const;
	UBO MakeUBO(const std::string& block, UBO::BufferTy data) const;

	//Set the order of textures in the associated material
	void TextureOrder(const std::vector<std::string>& order);

	using PersistCategory = ResourcePersistTag;

private:
	ShaderProgram(std::shared_ptr<ShaderResource> ptr);

	GLuint program;
	std::shared_ptr<ShaderResource> resource;
    HAS_HASH
};

MEMBER_HASH(ShaderProgram, program)

struct UBO::Proxy
{
	Proxy(UBO& ubo, Uniform unif) : ubo(ubo), unif(unif) {}
	
	template<typename Scalar, int Rows, int Cols>
	operator Eigen::Matrix<Scalar, Rows, Cols>() const
	{
		return const_cast<Proxy*>(this)->Map<Scalar>(Rows, Cols);
	}

	template<typename Derived>
	Proxy& operator=(const Eigen::MatrixBase<Derived>& data)
	{
		Map<Derived::Scalar>(data.rows(), data.cols()) = data;
		ubo.Sync();
		return *this;
	}

	operator std::uint32_t() const;
	Proxy& operator=(const std::uint32_t&);

	Proxy operator[](GLuint offset);

	template<typename Scalar>
	Eigen::Map<Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>>
	Map(std::ptrdiff_t Rows, std::ptrdiff_t Cols)
	{
		CheckType(UniformScalar(Scalar{}), Rows, Cols);
		return{ Ptr(Scalar{}), Rows, Cols };
	}

private:
	UBO& ubo;
	const Uniform unif;

	void CheckType(GLenum scalar, std::ptrdiff_t Rows, std::ptrdiff_t Cols) const;

	//Warning: these violate const correctness
	unsigned int* Ptr(unsigned int) const;
	int* Ptr(int) const;
	float* Ptr(float) const;
};

MEMBER_HASH(UBO, impl)

#endif
