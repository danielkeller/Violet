#include "stdafx.h"
#include "Render.hpp"

#include "Position.hpp"
#include "Mobile.hpp"
#include "File/Persist.hpp"

Render::Render(Position& position)
	: position(position), mobile(position)
	, instanceBuffer()
{}

template<class BufferObjTy>
void Render::FixInstances(render_data_t& dat, BufferObjTy& buf)
{
	GLsizei offset = 0;

	for (auto& shader : dat)
		for (auto& ss : dat.children<MatLevel>(shader))
			for (auto& vao : dat.children<VAOLevel>(ss))
			{
				auto numInstances =
					static_cast<GLsizei>(dat.children<InstanceLevel>(vao).size());
				vao.first.BindInstanceData(shader.first, buf, offset, numInstances);
				offset += numInstances;
			}
}

void Render::InternalCreate(Object obj, Material mat, VertexData vertData)
{
	auto refs = renderData.emplace(mat.shader, mat, std::tie(mat.shader, vertData),
		InstData{ obj, position.Get(obj).ToMatrix() });

	objs.insert(std::make_pair(obj, refs));

	FixInstances(renderData, instanceBuffer);
	//send the actual data each draw call
	instanceBuffer.Data(renderData.get_level<InstanceLevel>().size());
}

void Render::InternalCreateStatic(Object obj, Material mat, VertexData vertData)
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

	auto inst = InstData{ obj, position.Get(obj).ToMatrix() };
	auto refs = staticRenderData.emplace(mat.shader, mat, std::tie(mat.shader, vertData), inst);

	staticObjs.insert(std::make_pair(obj, refs));

	auto instref = std::get<InstanceLevel>(refs);
	position.Watch(obj, make_magic(locaccesor, instref));

	FixInstances(staticRenderData, staticInstanceBuffer);
	staticInstanceBuffer.Data(staticRenderData.get_level<InstanceLevel>().vector());
}

void Render::Create(Object obj, std::tuple<Material, VertexData, Mobilty> tup)
{
	Create(obj, std::get<0>(tup), std::get<1>(tup), std::get<2>(tup));
}

void Render::Create(Object obj, Material mat, VertexData vertData, Mobilty mobile)
{
	if (mobile == Mobilty::Yes)
		InternalCreate(obj, mat, vertData);
	else
		InternalCreateStatic(obj, mat, vertData);
}

void Render::DrawBucket(render_data_t& dat)
{
	for (auto& shader : dat)
	{
		shader.first.use();
		for (auto& ss : dat.children<MatLevel>(shader))
		{
			ss.first.use();
			for (auto& vao : dat.children<VAOLevel>(ss))
			{
				vao.first.Draw();
			}
		}
	}
}

void Render::Draw(float alpha)
{
	auto vec = renderData.get_level<InstanceLevel>().vector();
	mobile.Update(alpha, vec.begin(), vec.end());
	instanceBuffer.Data(vec);

	DrawBucket(renderData);
	DrawBucket(staticRenderData);
}

template<>
const Schema AttribTraits<InstData>::schema = {
    { "transform", GL_FLOAT, false, 0, {4, 4}, 4 * sizeof(float) },
    { "object", GL_UNSIGNED_INT, true, 16 * sizeof(float), {1, 1}, 0 },
};

void Render::Load(Persist& persist)
{
	for (const auto& row : persist.GetAll<Render>())
		Create(std::get<0>(row), std::get<2>(row), std::get<3>(row),
			std::get<1>(row) ? Mobilty::Yes : Mobilty::No);
}

void Render::Unload(Persist& persist)
{
	for (const auto& row : persist.GetAll<Render>())
		Remove(std::get<0>(row));
}

bool Render::Has(Object obj) const
{
	return objs.count(obj) || staticObjs.count(obj);
}

void Render::Save(Object obj, Persist& persist) const
{
	if (objs.count(obj))
	{
		auto refs = objs.find(obj)->second;
		persist.Set<Render>(obj, true,
			renderData.find<MatLevel>(std::get<MatLevel>(refs))->first,
			renderData.find<VAOLevel>(std::get<VAOLevel>(refs))->first.GetVertexData());
	}
	else if (staticObjs.count(obj))
	{
		auto refs = staticObjs.find(obj)->second;
		persist.Set<Render>(obj, false,
			staticRenderData.find<MatLevel>(std::get<MatLevel>(refs))->first,
			staticRenderData.find<VAOLevel>(std::get<VAOLevel>(refs))->first.GetVertexData());
	}
	else
		persist.Delete<Render>(obj);
}

void Render::Remove(Object obj)
{
	if (objs.count(obj))
	{
		auto refs = objs.find(obj)->second;
		renderData.erase(refs);
		objs.erase(obj);

		FixInstances(renderData, instanceBuffer);
		instanceBuffer.Data(renderData.get_level<InstanceLevel>().size());
	}
	else if (staticObjs.count(obj))
	{
		auto refs = staticObjs.find(obj)->second;
		staticRenderData.erase(refs);
		staticObjs.erase(obj);

		FixInstances(staticRenderData, staticInstanceBuffer);
		staticInstanceBuffer.Data(staticRenderData.get_level<InstanceLevel>().vector());
	}
}

std::tuple<Material, VertexData, Mobilty> Render::Info(Object obj)
{
	if (objs.count(obj))
	{
		auto refs = objs.find(obj)->second;
		return std::make_tuple(
			renderData.find<MatLevel>(std::get<MatLevel>(refs))->first,
			renderData.find<VAOLevel>(std::get<VAOLevel>(refs))->first.GetVertexData(),
			Mobilty::Yes);
	}
	else if (staticObjs.count(obj))
	{
		auto refs = staticObjs.find(obj)->second;
		return std::make_tuple(
			staticRenderData.find<MatLevel>(std::get<MatLevel>(refs))->first,
			staticRenderData.find<VAOLevel>(std::get<VAOLevel>(refs))->first.GetVertexData(),
			Mobilty::No);
	}
	throw std::domain_error(to_string(obj) + " is not being rendered");
}

template<>
const char* PersistSchema<Render>::name = "render";
template<>
Columns PersistSchema<Render>::cols = { "object", "static", "shader", "ss", "vertdata" };