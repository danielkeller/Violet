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

Render::LocationProxy::LocationProxy(
	std::shared_ptr<VAO::InstanceBuf> buf, VAO::InstanceBuf::perma_ref obj)
	: buf(buf), obj(obj)
{}

Render::LocationProxy::LocationProxy(const LocationProxy&& other)
	: buf(other.buf), obj(other.obj)
{}

Matrix4f& Render::LocationProxy::operator*()
{
	return *obj.get(*buf);
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
		vec.emplace_back(std::move(val));
		it = vec.end() - 1;
	}
	return it;
}

Render::LocationProxy Render::Create(Object obj, ShaderProgram shader, UBO ubo, std::vector<Tex> texes,
	VAO vao, const Matrix4f& loc)
{
	auto shaderit   = createOrAdd(shaders,             std::move(shader));
	auto materialit = createOrAdd(shaderit->materials, std::move(std::forward_as_tuple(ubo, texes)));
	auto shapeit    = createOrAdd(materialit->shapes, std::move(vao));

	auto objRef = shapeit->vao.InstanceBuffer().Container().emplace_back(loc);
	shapeit->vao.InstanceBuffer().Sync();

	return LocationProxy{ shapeit->vao.InstanceBuffer().ContainerPtr(), objRef };
}

void Render::draw() const
{
	//clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	commonUBO["camera"] = camera;
	commonUBO.Sync();

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
	//FIXME: Performance!
	vao.InstanceBuffer().Sync();
	vao.draw(static_cast<GLuint>(vao.InstanceBuffer().Container().size()));
}
