#include "stdafx.h"
#include "Render.hpp"

#include "Position.hpp"
#include "Mobile.hpp"
#include "File/Persist.hpp"

using namespace Render_detail;

Render::Render(Position& position)
	: position(position), mobile(position)
{}

template<GLenum bufferUsage>
void Render::Bucket<bufferUsage>::FixInstances()
{
	GLsizei offset = 0;

	for (auto& shader : data)
		for (auto& ss : data.children<MatLevel>(shader))
			for (auto& vao : data.children<VAOLevel>(ss))
			{
				auto numInstances =
					static_cast<GLsizei>(data.children<InstanceLevel>(vao).size());
				vao.first.BindInstanceData(shader.first, instances, offset, numInstances);
				offset += numInstances;
			}
}

template<GLenum bufferUsage>
render_data_t::perma_refs_t
Render::Bucket<bufferUsage>::Create(
	Object obj, Material mat,
	VertexData vertData, const InstData& inst)
{
	auto refs = data.emplace(mat.Shader(), mat, std::tie(mat.Shader(), vertData), inst);
	objs.insert(std::make_pair(obj, refs));
	FixInstances();
	return refs;
}

void Render::InternalCreate(Object obj, Material mat, VertexData vertData)
{
	mBucket.Create(obj, mat, vertData, InstData{ obj, position.Get(obj).ToMatrix() });
	//send the actual data each draw call
	mBucket.instances.Data(mBucket.data.get_level<InstanceLevel>().size());
}

void Render::InternalCreateStatic(Object obj, Material mat, VertexData vertData)
{
	using InstPermaRef = render_data_t::perma_ref_t<InstanceLevel>;
	static accessor<Transform, InstPermaRef> locaccesor = 
		[this](InstPermaRef ref, const Transform& v) mutable
		{
			auto it = sBucket.data.find<InstanceLevel>(ref);
			it->mat = v.ToMatrix();
			size_t offset = it - sBucket.data.begin<InstanceLevel>();
			sBucket.instances.Assign(offset, *it);
		};

	auto inst = InstData{ obj, position.Get(obj).ToMatrix() };
	auto refs = sBucket.Create(obj, mat, vertData, inst);

	auto instref = std::get<InstanceLevel>(refs);
	position.Watch(obj, make_magic(locaccesor, instref));
	//send the data now
	sBucket.instances.Data(sBucket.data.get_level<InstanceLevel>().vector());
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

template<GLenum bufferUsage>
void Render::Bucket<bufferUsage>::Draw()
{
	for (auto& shader : data)
	{
		shader.first.use();
		for (auto& ss : data.children<MatLevel>(shader))
		{
			ss.first.use();
			for (auto& vao : data.children<VAOLevel>(ss))
			{
				vao.first.Draw();
			}
		}
	}
}

void Render::Draw(float alpha)
{
	auto vec = mBucket.data.get_level<InstanceLevel>().vector();
	mobile.Update(alpha, vec.begin(), vec.end());
	mBucket.instances.Data(vec);

	mBucket.Draw(); sBucket.Draw();
}

template<>
const Schema AttribTraits<InstData>::schema = {
	AttribProperties{ "transform", GL_FLOAT, false, 0, {4, 4}, 4 * sizeof(float) },
	AttribProperties{ "object", GL_UNSIGNED_INT, true, 16 * sizeof(float), {1, 1}, 0 },
};

void Render::Load(const Persist& persist)
{
	for (const auto& row : persist.GetAll<Render>())
		Create(std::get<0>(row), std::get<2>(row), std::get<3>(row),
			std::get<1>(row) ? Mobilty::Yes : Mobilty::No);
}

void Render::Unload(const Persist& persist)
{
	for (const auto& row : persist.GetAll<Render>())
		Remove(std::get<0>(row));
}

bool Render::Has(Object obj) const
{
	return mBucket.objs.count(obj) || sBucket.objs.count(obj);
}

template<GLenum bufferUsage>
void Render::Bucket<bufferUsage>::Save(Object obj, bool mobile, Persist& persist) const
{
	auto refs = objs.find(obj)->second;
	persist.Set<Render>(obj, mobile,
		data.find<MatLevel>(std::get<MatLevel>(refs))->first,
		data.find<VAOLevel>(std::get<VAOLevel>(refs))->first.GetVertexData());
}

void Render::Save(Object obj, Persist& persist) const
{
	if (mBucket.objs.count(obj))
		mBucket.Save(obj, true, persist);
	else if (sBucket.objs.count(obj))
		sBucket.Save(obj, false, persist);
	else
		persist.Delete<Render>(obj);
}

template<GLenum bufferUsage>
void Render::Bucket<bufferUsage>::Remove(Object obj)
{
	auto refs = objs.find(obj)->second;
	data.erase(refs);
	objs.erase(obj);
	FixInstances();
}

void Render::Remove(Object obj)
{
	if (mBucket.objs.count(obj))
	{
		mBucket.Remove(obj);
		mBucket.instances.Data(mBucket.data.get_level<InstanceLevel>().size());
	}
	else if (sBucket.objs.count(obj))
	{
		sBucket.Remove(obj);
		sBucket.instances.Data(sBucket.data.get_level<InstanceLevel>().vector());
	}
}

template<GLenum bufferUsage>
std::tuple<Material, VertexData> Render::Bucket<bufferUsage>::Info(Object obj)
{
	auto refs = objs.find(obj)->second;
	return std::make_tuple(
		data.find<MatLevel>(std::get<MatLevel>(refs))->first,
		data.find<VAOLevel>(std::get<VAOLevel>(refs))->first.GetVertexData());
}

std::tuple<Material, VertexData, Mobilty> Render::Info(Object obj)
{
	if (mBucket.objs.count(obj))
		return std::tuple_cat(mBucket.Info(obj), std::make_tuple(Mobilty::Yes));
	else if (sBucket.objs.count(obj))
		return std::tuple_cat(sBucket.Info(obj), std::make_tuple(Mobilty::No));
	throw std::domain_error(to_string(obj) + " is not being rendered");
}

template<>
const char* PersistSchema<Render>::name = "render";
template<>
Columns PersistSchema<Render>::cols = { "object", "static", "shader", "ss", "vertdata" };