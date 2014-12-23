#include "stdafx.h"
#include "Render.hpp"
#include <algorithm>

Render::Render()
	: simpleShader("assets/simple")
	, commonUBO(simpleShader.MakeUBO("Common", "Common"))
	, instanceBuffer()
{
	//Assumes no one binds over UBO::Common
	commonUBO.Bind();
}

Render::LocationProxy::LocationProxy(
	std::weak_ptr<Shape::InstanceVec> buf, Shape::InstanceVec::perma_ref obj)
	: buf(buf), obj(obj)
{}

Render::LocationProxy::LocationProxy(const LocationProxy&& other)
	: buf(other.buf), obj(other.obj)
{}

Matrix4f& Render::LocationProxy::operator*()
{
	return *buf.lock()->get(obj);
}

Render::Shape::Shape(Shape&& other)
	: vao(std::move(other.vao))
{}

template<typename Cont, typename Val>
typename Cont::iterator createOrAdd(Cont& vec, Val&& val)
{
	auto it = std::find(vec.begin(), vec.end(), val);
	if (it == vec.end())
	{
		vec.emplace_back(std::forward<Val>(val));
		it = vec.end() - 1;
	}
	return it;
}

Render::LocationProxy Render::Create(Object obj, ShaderProgram shader, UBO ubo,
	std::vector<Tex> texes, VertexData vertData, const Matrix4f& loc)
{
	auto shaderit   = createOrAdd(shaders,             shader);
	auto materialit = createOrAdd(shaderit->materials, std::make_tuple(ubo, texes));
	auto shapeit    = createOrAdd(materialit->shapes,  std::make_tuple(vertData, shader));

	auto objRef = shapeit->instances->emplace_back(loc);

	size_t offset = 0;
	for (auto& shader : shaders)
		for (auto& material : shader.materials)
			for (auto& shape : material.shapes)
			{
				shape.vao.BindInstanceData(shader.program, instanceBuffer, offset,
					shapeit->instances->size());
				offset += shapeit->instances->size();
			}

	instanceBuffer.Data(offset);

	return LocationProxy{ shapeit->instances, objRef };
}
void Render::draw() const
{
	//clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	commonUBO["camera"] = camera;
	commonUBO.Sync();

	{
		auto mapping = instanceBuffer.Map(GL_WRITE_ONLY);
		auto mapit = mapping.begin();
		for (auto& shader : shaders)
			for (auto& material : shader.materials)
				for (auto& shape : material.shapes)
					mapit = std::copy(shape.instances->begin(), shape.instances->end(), mapit);
	} //unmap

	for (auto& shader : shaders) shader.draw();
}

void Render::Shader::draw() const
{
	program.use();
	for (auto& material : materials) material.draw();
}

void Render::Material::draw() const
{
	for (GLuint i = 0; i < textures.size(); ++i)
		textures[i].Bind(i);

	materialProps.Bind();

	for (auto& shape : shapes) shape.draw();
}

void Render::Shape::draw() const
{
	vao.Draw();
}

const Schema Render::InstData::schema = {
	{ "transform", 4, GL_FLOAT, 0, 4, 4 * sizeof(float) },
};
