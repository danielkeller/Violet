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
typename std::vector<T>::iterator createOrAdd(std::vector<T>& vec, U&& val)
{
	auto it = std::find(vec.begin(), vec.end(), val);
	if (it == vec.end())
	{
		vec.emplace_back(std::move(val));
		it = vec.end() - 1;
	}
	return it;
}

void Render::Create(Object obj, ShaderProgram&& shader, std::tuple<UBO, std::vector<Tex>>&& mat, 
	VAO&& vao, const Matrix4f& loc)
{
	auto shaderit   = createOrAdd(shaders,             std::move(shader));
	auto materialit = createOrAdd(shaderit->materials, std::move(mat));
	auto shapeit    = createOrAdd(materialit->shapes,  std::move(vao));

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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
	for (auto& material : materials) material.draw();
}

void Render::Material::draw() const
{
	for (size_t i = 0; i < textures.size(); ++i)
		textures[i].Bind(i);

	materialProps.Bind();

	for (auto& shape : shapes) shape.draw();
}

void Render::Shape::draw() const
{
	vao.bind();
	vao.draw(locations.size());
}

template<>
Schema ArrayBuffer<Render::ObjectLocation>::schema = {
		{ "transform", 4, GL_FLOAT, 0, 4, 4 * sizeof(float) },
};