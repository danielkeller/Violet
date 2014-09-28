#include "stdafx.h"
#include "Render.hpp"
#include <algorithm>

Render::Render()
	: simpleShader(ShaderProgram::create("assets/simple"))
	, commonUBO(simpleShader.GetUBO("Common"))
{
	//Assumes no one binds over UBO::Common
	commonUBO.Bind();
}

template<typename T, typename U>
typename std::vector<T>::iterator createOrAdd(std::vector<T>& vec, U val)
{
	auto it = std::find(vec.begin(), vec.end(), val);
	if (it == vec.end())
	{
		vec.emplace_back(val);
		it = vec.end() - 1;
	}
	return it;
}

void Render::Create(Object obj, ShaderProgram::Ref shader, VAO::Ref vao, const Matrix4f& loc)
{
	auto shaderit = createOrAdd(shaders, shader);

	//dummy materials for now
	if (shaderit->materials.empty())
		shaderit->materials.emplace_back();
	auto materialit = shaderit->materials.begin();

	auto shapeit = createOrAdd(materialit->shapes, vao);

	shapeit->locations.emplace_back(ObjectLocation{ loc, obj });

	//Have we just added this shape?
	if (shaderit->materials.size() == 1 && materialit->shapes.size() == 1)
	{
		//This info is part of VAO state
		shapeit->vao.bind();
		shapeit->instances.BindToShader(shader);
	}

	shapeit->instances.Data(shapeit->locations);
}

void Render::draw() const
{
	//clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT);

	commonUBO["camera"] = camera;
	commonUBO.Sync();

	for (auto& shader : shaders) shader.draw();
}

Render::Shape::Shape(Shape&& other)
	: instances(std::move(other.instances))
	, vao(std::move(other.vao))
	, locations(std::move(other.locations))
{}

void Render::Shader::draw() const
{
	program.use();
	for (auto& material : materials) material.draw(program);
}

void Render::Material::draw(const ShaderProgram::Ref& program) const
{
	for (auto& shape : shapes) shape.draw(program);
}

void Render::Shape::draw(const ShaderProgram::Ref& program) const
{
	vao.bind();
	vao.draw(locations.size());

	//find the uniform variable called modelView
	/*
	GLint posUnif = program.GetUniformLocation("transform");

	//will be replaced with instancing
	for (auto& loc : locations)
	{
		//set it to the matrix
		glUniformMatrix3fv(posUnif, 1, GL_FALSE, loc.loc.data());

		vao.draw();
	}*/
}

template<>
Schema ArrayBuffer<Render::ObjectLocation>::schema = {
		{ "transform", 4, GL_FLOAT, 0, 4, 4 * sizeof(float) },
};