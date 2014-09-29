#ifndef RENDER_H
#define RENDER_H

#include "stdafx.h"
#include "Shader.hpp"
#include "Object.hpp"
#include "Uniform.hpp"
#include "VAO.hpp"
#include "Texture.hpp"

#include <vector>
#include <map>
#include <memory>

#include <iostream>

//Instead of including window's GLU, just define the one useful function
//and link against it
extern "C" const GLubyte* APIENTRY gluErrorString(GLenum errCode);

inline void printErr()
{
	if (GLenum err = glGetError())
		std::cerr << "GL Error '" << gluErrorString(err) << "'\n";
}

class Render
{
public:
	void Create(Object obj, ShaderProgram&& shader, std::tuple<UBO, std::vector<Tex>>&& mat,
		VAO&& vao, const Matrix4f& loc);
	void Destroy(Object obj);
	void draw() const;
	Matrix4f camera;

	Render();

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
	//UBO shared with all shaders
	ShaderProgram simpleShader;
	mutable UBO commonUBO;

	struct ObjectLocation
	{
		//Prevent alignment issues from causing asserts
		using Allocator = Eigen::aligned_allocator<ObjectLocation>;

		Matrix4f loc;
		Object owner;
	};

	struct Shape
	{
		ArrayBuffer<ObjectLocation> instances;
		VAO vao;
		std::vector<ObjectLocation, ObjectLocation::Allocator> locations;
		void draw() const;

		Shape(VAO&& vao)
			: vao(std::move(vao)), instances(1, GL_DYNAMIC_DRAW) {}

		Shape(const Shape&) = delete;
		Shape(Shape&&); //MSVC sucks and can't default this

		bool operator==(const VAO& other)
			{ return vao == other; }
		bool operator!=(const VAO& other)
			{ return !(vao == other); }
	};

	struct Material
	{
		std::vector<Shape> shapes;
		UBO materialProps;
		std::vector<Tex> textures;
		void draw() const;

		bool operator==(const std::tuple<UBO, std::vector<Tex>>& t)
		{
			return std::tie(materialProps, textures) == t;
		}
		bool operator!=(const std::tuple<UBO, std::vector<Tex>>& t)
		{
			return !(*this == t);
		}
		
		Material(std::tuple<UBO, std::vector<Tex>>&& t)
			: materialProps(std::move(std::get<0>(t)))
			, textures(std::move(std::get<1>(t)))
		{}
		Material(const Material&) = delete;
		Material(Material&& other)
			: shapes(std::move(other.shapes))
			, materialProps(std::move(other.materialProps))
			, textures(std::move(other.textures))
		{}
	};

	struct Shader
	{
		ShaderProgram program;
		std::vector<Material> materials;
		void draw() const;

		Shader(ShaderProgram&& program)
			: program(std::move(program)) {}

		bool operator==(const ShaderProgram& other)
			{ return program == other; }
		bool operator!=(const ShaderProgram& other)
			{ return !(program == other); }

		Shader(const Shader&) = delete;
		Shader(Shader&& other)
			: materials(std::move(other.materials)), program(std::move(other.program))
		{}
	};

	std::vector<Shader> shaders;
};

#endif