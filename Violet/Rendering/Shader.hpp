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

struct UniformBlock
{
	UniformBlock() : byte_size(0) {}

	std::string name;
	size_t byte_size;
	std::vector<Uniform> uniforms;

	MEMBER_EQUALITY(std::string, name)

	const Uniform& operator[](const std::string& name) const;
	Uniform& operator[](const std::string& name)
	{
		return const_cast<Uniform&>(const_cast<const UniformBlock&>(*this)[name]);
	}
};

class ShaderProgram;

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
	using BufferObjTy = BufferObject<BufferElemTy, GL_UNIFORM_BUFFER, GL_STREAM_DRAW>;

public:
	using BufferTy = std::vector<BufferElemTy>;
	using BindProxy = BufferObjTy::IndexedBindProxy;

	UBO(ShaderProgram, const std::string& block);
	UBO(ShaderProgram, const std::string& block, BufferTy data);
	UBO(const UBO&) = delete;
	UBO(UBO&&);

	Proxy operator[](const std::string& name);

	//Associates this UBO with its binding point.
	void Bind() const;
	BindProxy GetBindProxy() const;

	const BufferTy& Data() const;
	std::vector<Uniform> Uniforms() const;

	//Only needed when editing the data through []::Map()
	void Sync();

private:
	UBO(UniformBlock block, BufferTy data);

	BufferTy data;
	BufferObjTy bufferObject;
	//Type allows us to distinguish between UBOs that are shared (ie, camera)
	//with those that are not (ie, material).
	Type type;
	UniformBlock block;
};

struct ResourcePersistTag;
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
	ShaderProgram(const char* path) : ShaderProgram(std::string(path)) {}
	std::string Name() const;
	BASIC_EQUALITY(ShaderProgram, program)

	//Enable this program for rendering
	void use() const;

	//get an attribute
	GLint GetAttribLocation(const std::string& name) const;
	GLenum GetAttribType(GLint loc) const;

	//Set the order of textures in the associated material
	void TextureOrder(const std::vector<std::string>& order);

	using PersistCategory = ResourcePersistTag;

private:
	ShaderProgram(std::shared_ptr<ShaderResource> ptr);

	GLuint program;
	std::shared_ptr<ShaderResource> resource;
	friend class UBO;
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
		Map<typename Derived::Scalar>(data.rows(), data.cols()) = data;
		ubo.Sync();
		return *this;
	}

	operator std::uint32_t() const;
    Proxy& operator=(std::uint32_t);
    operator float() const;
    Proxy& operator=(float);
    
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

#endif
