#ifndef SHADER_H
#define SHADER_H

#include "Core/Resource.hpp"
#include "BufferObject.hpp"

#include <cstdint>

struct Uniform
{
	std::string name;
	MEMBER_EQUALITY(std::string, name)
	GLenum type;
	GLuint size, offset;
	GLint stride, location;
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

private:
	using BufferObjTy = BufferObject<BufferElemTy, GL_UNIFORM_BUFFER, GL_STREAM_DRAW>;
	struct Impl;
	BufferObjTy::IndexedBindProxy bindProxy;
	std::shared_ptr<Impl> impl;

	UBO(std::shared_ptr<Impl>);

	void Sync();
	HAS_HASH
};

struct ResourcePersistTag;
struct EmbeddedResourcePersistTag;
class Persist;

//FIXME: static instances of this class can cause crashes at exit if the
//destructor happens to be called after the gl module is unloaded and
//glDeleteProgram hasn't been called (ie, loaded) yet
class ShaderProgram
{
	struct ShaderResource;

public:
    ShaderProgram() : ShaderProgram("assets/blank") {}

	ShaderProgram(std::string path);

	//Enable this program for rendering
	void use() const;

	//get an attribute
	GLint GetAttribLocation(const std::string& name) const;
	GLenum GetAttribType(GLint loc) const;

	BASIC_EQUALITY(ShaderProgram, program)

	std::string Name() const;
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
		Scalar dummy{};
		CheckType(dummy, Rows, Cols);
		return Eigen::Map<Eigen::Matrix<Scalar, Rows, Cols>>(Ptr(dummy));
	}

	template<typename Derived>
	Proxy& operator=(const Eigen::MatrixBase<Derived>& data)
	{
		Derived::Scalar dummy{};
		CheckType(dummy, data.rows(), data.cols());
		Eigen::Map<Eigen::MatrixBase<Derived>::PlainObject>(
			Ptr(dummy), data.rows(), data.cols()) = data;
		ubo.Sync();
		return *this;
	}

	operator std::uint32_t() const;
	Proxy& operator=(const std::uint32_t&);

	Proxy operator[](GLuint offset);

private:
	UBO& ubo;
	const Uniform unif;

	void CheckType(GLenum type) const;
	void CheckType(float dummy, std::ptrdiff_t Rows, std::ptrdiff_t Cols) const;
	void CheckType(int dummy, std::ptrdiff_t Rows, std::ptrdiff_t Cols) const;
	void CheckType(unsigned int dummy, std::ptrdiff_t Rows, std::ptrdiff_t Cols) const;

	//Warning: these violate const correctness
	unsigned int* Ptr(unsigned int) const;
	int* Ptr(int) const;
	float* Ptr(float) const;
};

MEMBER_HASH(UBO, impl)

#endif
