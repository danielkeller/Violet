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

	static const int ShaderLevel = 0, MatLevel = 1, VAOLevel = 2, InstanceLevel = 3;
	using render_data_t = tuple_tree<ShaderProgram, Material, VAO, InstData>;

	render_data_t staticRenderData;
	BufferObject<InstData, GL_ARRAY_BUFFER, GL_STATIC_DRAW> staticInstanceBuffer;

	std::unordered_map<Object, render_data_t::perma_refs_t> staticObjs;

	//maybe:
	//l_unordered_map<ShaderProgram, l_unordered_map<Material,
	//	l_unordered_map<VertexData, Render_detail::Shape>>> renderData;

	render_data_t renderData;
    BufferObject<InstData, GL_ARRAY_BUFFER, GL_STREAM_DRAW> instanceBuffer;

	std::unordered_map<Object, render_data_t::perma_refs_t> objs;

	void InternalCreateStatic(Object obj, Material mat, VertexData vertData);

	void InternalCreate(Object obj, Material mat, VertexData vertData);

	template<class BufferObjTy>
	void FixInstances(render_data_t& dat, BufferObjTy& buf);

	void DrawBucket(render_data_t& dat);
};

MAKE_PERSIST_TRAITS(Render, Object, bool, Material, VertexData);

#endif
