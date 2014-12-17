#ifndef SHADER_H
#define SHADER_H

#include "Resource.hpp"
#include "BufferObject.hpp"

#include <istream>
#include <memory>

class UBO;

class ShaderProgram
{
	struct ShaderResource;

public:
	ShaderProgram(std::string path);

	//Enable this program for rendering
	void use() const;

	//get an attribute
	GLint GetAttribLocation(const char* name) const;
	GLenum GetAttribType(GLint loc) const;

	BASIC_EQUALITY(ShaderProgram, program)

	std::string Name() const;
	//make a new UBO, or find an existing one
	UBO MakeUBO(const std::string& block, const std::string& name) const;
	//Set the order of textures in the associated material
	void TextureOrder(const std::vector<std::string>& order);

private:
	ShaderProgram(std::shared_ptr<ShaderResource> ptr);
	struct ShaderResource;

	GLuint program;
	std::shared_ptr<ShaderResource> resource;
};

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
	using BufferTy = float; //for alignment
	using BufferObjTy = MutableBufferObject<std::vector<BufferTy>, GL_UNIFORM_BUFFER>;

	struct UBOResource;

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

#endif
