#ifndef RENDER_H
#define RENDER_H

#include "stdafx.h"
#include "Shader.hpp"
#include "Object.hpp"
#include "Uniform.hpp"
#include "VAO.hpp"

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
	void Create(Object obj, ShaderProgram::Ref shader, VAO::Ref vao, const Matrix4f& loc);
	void Destroy(Object obj);
	void draw() const;
	Matrix4f camera;

	Render();

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
	//UBO shared with all shaders
	ShaderProgram::Ref simpleShader;
	mutable UBO commonUBO;

	struct ObjectLocation
	{
		EIGEN_MAKE_ALIGNED_OPERATOR_NEW
		Matrix4f loc;
		Object owner;
	};

	struct Shape
	{
		ArrayBuffer<ObjectLocation> instances;
		VAO::Ref vao;
		std::vector<ObjectLocation> locations;
		void draw(const ShaderProgram::Ref& program) const;

		Shape(VAO::Ref vao)
			: vao(vao), instances(1, GL_DYNAMIC_DRAW) {}

		Shape(const Shape&) = delete;
		Shape(Shape&&); //MSVC sucks and can't default this

		bool operator==(const VAO::Ref& other)
			{ return vao == other; }
		bool operator!=(const VAO::Ref& other)
			{ return !(vao == other); }
	};

	struct Material
	{
		std::vector<Shape> shapes;
		void draw(const ShaderProgram::Ref& program) const;
		
		Material() = default;
		Material(const Material&) = delete;
		Material(Material&& other) : shapes(std::move(other.shapes))
		{}
	};

	struct Shader
	{
		ShaderProgram::Ref program;
		std::vector<Material> materials;
		void draw() const;

		Shader(ShaderProgram::Ref program)
			: program(program) {}

		bool operator==(const ShaderProgram::Ref& other)
			{ return program == other; }
		bool operator!=(const ShaderProgram::Ref& other)
			{ return !(program == other); }

		Shader(const Shader&) = delete;
		Shader(Shader&& other)
			: materials(std::move(other.materials)), program(std::move(other.program))
		{}
	};

	std::vector<Shader> shaders;
};

#endif