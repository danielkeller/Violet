#include "stdafx.h"
#include "Render.hpp"

#include "Position.hpp"
#include "Mobile.hpp"
#include "Persist.hpp"

using namespace Render_detail;

Render::Render(Position& position, Mobile& m, Persist& persist)
	: position(position), m(m), persist(persist)
	, simpleShader("assets/simple")
	, commonUBO(simpleShader.MakeUBO("Common", "Common"))
	, instanceBuffer()
{
	//Assumes no one binds over UBO::Common
	commonUBO.Bind();
}

void Render::Load()
{
	for (const auto& row : persist.GetAll<Render>())
	{
		Create(std::get<0>(row), std::get<2>(row), std::get<3>(row), std::get<4>(row),
			std::get<1>(row) ? Mobilty::Yes : Mobilty::No);
	}
}

//this really should be its own data structure
template<class data_t, class lower_data_t>
std::pair<typename lower_data_t::iterator, typename lower_data_t::iterator>
range_of(typename data_t::iterator it, data_t& data, lower_data_t& lower_data)
{
    if (it == data.end())
        return {lower_data.end(), lower_data.end()};
    else if (it + 1 == data.end())
        return {lower_data.find(it[0].begin), lower_data.end()};
    else
		return{ lower_data.find(it[0].begin), lower_data.find(it[1].begin) };
}

template<class BufferObjTy>
void Render::FixInstances(render_data_t& dat, BufferObjTy& buf)
{
	GLsizei offset = 0;

	for (auto& shader : dat)
		for (auto& mat : dat.children<MatLevel>(shader))
			for (auto& vao : dat.children<VAOLevel>(mat))
			{
				auto numInstances =
					static_cast<GLsizei>(dat.children<InstanceLevel>(vao).size());
				vao.first.BindInstanceData(shader.first, buf, offset, numInstances);
				offset += numInstances;
			}
}

void Render::InternalCreate(Object obj, ShaderProgram shader, Material mat, VertexData vertData)
{
	using InstPermaRef = render_data_t::perma_ref_t<InstanceLevel>;
	static accessor<Matrix4f, InstPermaRef> locaccesor = {
		[this](InstPermaRef ref) { return renderData.find<InstanceLevel>(ref)->mat; },
		[this](InstPermaRef ref, const Matrix4f& v)
		{
			renderData.find<InstanceLevel>(ref)->mat = v;
		}
	};

	auto refs = renderData.emplace(shader, mat, std::tie(shader, vertData),
		InstData{ obj, *m[obj] });

	objs.insert(std::make_pair(obj, refs));

	auto instref = std::get<InstanceLevel>(refs);
	m[obj] = make_magic(locaccesor, instref);

	FixInstances(renderData, instanceBuffer);
	//send the actual data each draw call
	instanceBuffer.Data(renderData.get_level<InstanceLevel>().size());
}

void Render::InternalCreateStatic(Object obj, ShaderProgram shader,
	Material mat, VertexData vertData)
{
	using InstPermaRef = render_data_t::perma_ref_t<InstanceLevel>;
	static accessor<Transform, InstPermaRef> locaccesor = 
		[this](InstPermaRef ref, const Transform& v) mutable
		{
			auto& it = staticRenderData.find<InstanceLevel>(ref);
			it->mat = v.ToMatrix();
			size_t offset = it - staticRenderData.begin<InstanceLevel>();
			staticInstanceBuffer.Assign(offset, *it);
		};

	auto inst = InstData{ obj, position[obj].get().ToMatrix() };
	auto refs = staticRenderData.emplace(shader, mat, std::tie(shader, vertData), inst);

	staticObjs.insert(std::make_pair(obj, refs));

	auto instref = std::get<InstanceLevel>(refs);
	position[obj] += make_magic(locaccesor, instref);

	FixInstances(staticRenderData, staticInstanceBuffer);
	staticInstanceBuffer.Data(staticRenderData.get_level<InstanceLevel>().vector());
}

void Render::Create(Object obj, ShaderProgram shader, Material mat,
	VertexData vertData, Mobilty mobile)
{
	if (mobile == Mobilty::Yes)
		InternalCreate(obj, shader, mat, vertData);
	else
		InternalCreateStatic(obj, shader, mat, vertData);
}

void Render::DrawBucket(render_data_t& dat)
{
	for (auto& shader : dat)
	{
		shader.first.use();
		for (auto& mat : dat.children<MatLevel>(shader))
		{
			mat.first.use();
			for (auto& vao : dat.children<VAOLevel>(mat))
			{
				vao.first.Draw();
			}
		}
	}
}

void Render::Draw()
{
	commonUBO["camera"] = camera;
	commonUBO.Sync();
	
	instanceBuffer.Data(renderData.get_level<InstanceLevel>().vector());

	DrawBucket(renderData);
	DrawBucket(staticRenderData);
}

template<>
const Schema AttribTraits<InstData>::schema = {
    { "transform", GL_FLOAT, false, 0, {4, 4}, 4 * sizeof(float) },
    { "object", GL_UNSIGNED_INT, true, 16 * sizeof(float), {1, 1}, 0 },
};

void Render::Save(Object obj)
{
	if (objs.count(obj))
	{
		auto refs = objs.find(obj)->second;
		persist.Set<Render>(obj, false,
			renderData.find<ShaderLevel>(std::get<ShaderLevel>(refs))->first,
			renderData.find<MatLevel>(std::get<MatLevel>(refs))->first,
			renderData.find<VAOLevel>(std::get<VAOLevel>(refs))->first.GetVertexData());
	}
	else if (staticObjs.count(obj))
	{
		auto refs = staticObjs.find(obj)->second;
		persist.Set<Render>(obj, false,
			staticRenderData.find<ShaderLevel>(std::get<ShaderLevel>(refs))->first,
			staticRenderData.find<MatLevel>(std::get<MatLevel>(refs))->first,
			staticRenderData.find<VAOLevel>(std::get<VAOLevel>(refs))->first.GetVertexData());
	}
}

template<>
const char* PersistSchema<Render>::name = "render";
template<>
Columns PersistSchema<Render>::cols = { "object", "static", "shader", "mat", "vertdata" };