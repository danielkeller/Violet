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

Render::LocationProxy::LocationProxy(Shape::InstanceBuf buf, Object obj, Render& r)
	: buf(buf), obj(obj), render(r)
{}

Render::LocationProxy::operator Matrix4f() const
{
	auto it = std::find_if(buf.Vector().begin(), buf.Vector().end(), [this](const ObjectLocation& loc) {
		return loc.owner == obj;
	});

	return it->loc;
}

void Render::LocationProxy::operator=(const Matrix4f& l)
{
	auto it = std::find_if(buf.Vector().begin(), buf.Vector().end(), [this](const ObjectLocation& loc) {
		return loc.owner == obj;
	});

	it->loc = l;
	render.dirtyBufs.insert(buf);
}

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

Render::LocationProxy Render::Create(Object obj, ShaderProgram shader, std::tuple<UBO, std::vector<Tex>> mat,
	VAO vao, const Matrix4f& loc)
{
	auto shaderit   = createOrAdd(shaders,             std::move(shader));
	auto materialit = createOrAdd(shaderit->materials, std::move(mat));
	auto shapeit   = createOrAdd(materialit->shapes, std::move(vao));

	shapeit->instances.Vector().emplace_back(ObjectLocation{ loc, obj });

	//Have we just added this shape?
	if (shaderit->materials.size() == 1 && materialit->shapes.size() == 1)
	{
		//This info is part of VAO state
		shapeit->vao.bind();
		shapeit->instances.BindToShader(shader);
	}

	shapeit->instances.Sync();

	return LocationProxy{ shapeit->instances, obj, *this };
}

void Render::draw() const
{
	//clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Sync array buffers
	for (const auto& buf : dirtyBufs) buf.Sync();
	dirtyBufs.clear();

	commonUBO["camera"] = camera;
	commonUBO.Sync();

	for (auto& shader : shaders) shader.draw();
}

Render::Shape::Shape(Shape&& other)
	: vao(std::move(other.vao))
	, instances(std::move(other.instances))
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
	//this only causes a cache miss if it has to sync.
	//Also, it should never need to drain the pipeline.
	instances.Sync();

	vao.bind();
	vao.draw(instances.Vector().size());
}

template<>
Schema ArrayBuffer<Render::ObjectLocation>::schema = {
		{ "transform", 4, GL_FLOAT, 0, 4, 4 * sizeof(float) },
};
