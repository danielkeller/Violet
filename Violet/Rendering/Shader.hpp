#ifndef SHADER_H
#define SHADER_H

#include "Resource.hpp"
#include "BufferObject.hpp"

#include <istream>
#include <memory>
#include <cstdint>

class UBO;

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
	//make a new UBO, or find an existing one
	UBO MakeUBO(const std::string& block, const std::string& name) const;
	//Set the order of textures in the associated material
	void TextureOrder(const std::vector<std::string>& order);

private:
	ShaderProgram(std::shared_ptr<ShaderResource> ptr);

	GLuint program;
	std::shared_ptr<ShaderResource> resource;
    HAS_HASH
};

MEMBER_HASH(ShaderProgram, program)

//Uniform Buffer Object
class UBO
{
	struct Proxy;
	enum Type
	{
		Common,
		Material
	};

public:
	//It should be possible to call Bind with no ill effects
	UBO()
		: bindProxy(Material)
	{}

	BASIC_EQUALITY(UBO, bindProxy)

	Proxy operator[](const std::string& name)
	{
		return Proxy(*this, name);
	}

	//synchronize with OpenGL. Note that this stalls anything using this UBO.
	void Sync() const;

	//Associates this UBO with its binding point.
	void Bind() const;

	friend struct Proxy;
	friend class ShaderProgram;

private:
	using BufferTy = char; //typename std::aligned_storage<sizeof(char), alignof(Matrix4f)>::type;
	using BufferObjTy = BufferObject<BufferTy, GL_UNIFORM_BUFFER, GL_STREAM_DRAW>;

	struct UBOResource;

	UBO(std::shared_ptr<UBOResource>);

	BufferObjTy::IndexedBindProxy bindProxy;
	std::shared_ptr<UBOResource> resource;

	struct Proxy
	{
		Proxy(UBO& ubo, const std::string& name)
			: ubo(ubo), name(name) {}

        explicit operator Vector3f() const;
        Proxy& operator=(const Vector3f&);
		explicit operator Matrix3f() const;
		Proxy& operator=(const Matrix3f&);
		explicit operator Matrix4f() const;
		Proxy& operator=(const Matrix4f&);
		explicit operator std::uint32_t() const;
		Proxy& operator=(const std::uint32_t&);

	private:
		UBO& ubo;
		const std::string& name;

		template<typename T, GLenum ty>
		T ConvertOpHelper() const;
		template<GLenum ty, typename T>
		UBO::Proxy& AssignOpHelper(const T&);
		template<typename T, GLenum ty>
		T ScalarConvertOpHelper() const;
		template<GLenum ty, typename T>
		UBO::Proxy& ScalarAssignOpHelper(const T&);
	};
    HAS_HASH
};

MEMBER_HASH(UBO, resource)

#endif
