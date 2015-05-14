#ifndef RENDER_H
#define RENDER_H

#include "Shader.hpp"
#include "Core/Object.hpp"
#include "VAO.hpp"
#include "RenderPasses.hpp"
#include "Material.hpp"
#include "Mobile.hpp"

#include "Containers/l_bag.hpp"
#include "Containers/tuple_tree.hpp"

#include "Core/Component.hpp"

#include <unordered_set>
#include <unordered_map>
#include <array>

class Position;
class Persist;

enum class Mobilty
{
	Yes,
	No
};

struct InstData
{
	Matrix4f mat;
	Object obj;

	InstData(const InstData&) = default;
	InstData(Object o) : mat(), obj(o) {}
	InstData(Object o, const Matrix4f& m) : mat(m), obj(o) {}
	InstData() : mat(), obj(Object::invalid) {}
	InstData& operator=(const Matrix4f& m) { mat = m; return *this; }

	MEMBER_EQUALITY(Object, obj);
	BASIC_EQUALITY(InstData, obj);
};

using InstanceVec = l_bag<InstData, Eigen::aligned_allocator<InstData>>;

namespace Render_detail
{
	static const int ShaderLevel = 0, MatLevel = 1, VAOLevel = 2, InstanceLevel = 3;
	using render_data_t = tuple_tree<ShaderProgram, Material, VAO, InstData>;
}

class Render : public Component
{
public:
	void Create(Object obj, Material mat, VertexData vertData, Mobilty mobile = Mobilty::No);
	void Create(Object obj, std::tuple<Material, VertexData, Mobilty>);

	void Load(Persist&);
	void Unload(Persist&);
	bool Has(Object obj) const;
	void Save(Object obj, Persist&) const;
	void Remove(Object obj);

	void Draw(float alpha);

	std::tuple<Material, VertexData, Mobilty> Info(Object obj);

	Render(Position&);
	Render(const Render&) = delete;
	void operator=(const Render&) = delete;

private:
	Position& position;
	Mobile mobile;

	template<GLenum bufferUsage>
	struct Bucket
	{
		using render_data_t = Render_detail::render_data_t;

		render_data_t data;
		BufferObject<InstData, GL_ARRAY_BUFFER, bufferUsage> instances;
		std::unordered_map<Object, render_data_t::perma_refs_t> objs;

		render_data_t::perma_refs_t
		Create(Object obj, Material mat, VertexData vertData, const InstData& inst);
		void FixInstances();
		void Draw();

		void Save(Object obj, bool mobile, Persist& persist) const;
		void Remove(Object obj);
		std::tuple<Material, VertexData> Info(Object obj);
	};

	Bucket<GL_STREAM_DRAW> mBucket;
	Bucket<GL_STATIC_DRAW> sBucket;

	void InternalCreateStatic(Object obj, Material mat, VertexData vertData);
	void InternalCreate(Object obj, Material mat, VertexData vertData);
};

MAKE_PERSIST_TRAITS(Render, Object, bool, Material, VertexData);

#endif
