#ifndef RENDER_H
#define RENDER_H

#include "Shader.hpp"
#include "Object.hpp"
#include "VAO.hpp"
#include "RenderPasses.hpp"
#include "Material.hpp"

#include "Containers/l_bag.hpp"
#include "Containers/tuple_tree.hpp"

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <array>

class Position;
class Mobile;
class Persist;

enum class Mobilty
{
	Yes,
	No
};

#include "Render_detail.hpp"

class Render
{
public:
	void Create(Object obj, ShaderProgram shader, Material mat,
		VertexData vertData, Mobilty mobile = Mobilty::No);
	//For things like text that use custom instance data
	//void Create(Object obj, ShaderProgram shader, Material mat, VAO vao);

	void Remove(Object obj);

	void Save(Object obj);

	void Draw();
	Matrix4f camera;

	Render(Position&, Mobile&, Persist&);
	Render(const Render&) = delete;
	void operator=(const Render&) = delete;

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
	Position& position;
	Mobile& m;
	Persist& persist;

	//UBO shared with all shaders
	ShaderProgram simpleShader;
    UBO commonUBO;

	static const int ShaderLevel = 0, MatLevel = 1, VAOLevel = 2, InstanceLevel = 3;
	using render_data_t = tuple_tree<ShaderProgram, Material, VAO, Render_detail::InstData>;

	render_data_t staticRenderData;
	BufferObject<Render_detail::InstData, GL_ARRAY_BUFFER, GL_STATIC_DRAW> staticInstanceBuffer;

	std::unordered_map<Object, render_data_t::perma_refs_t> staticObjs;

	//maybe:
	//l_unordered_map<ShaderProgram, l_unordered_map<Material,
	//	l_unordered_map<VertexData, Render_detail::Shape>>> renderData;

	render_data_t renderData;
    BufferObject<Render_detail::InstData, GL_ARRAY_BUFFER, GL_STREAM_DRAW> instanceBuffer;

	std::unordered_map<Object, render_data_t::perma_refs_t> objs;

	void InternalCreateStatic(Object obj, ShaderProgram shader, Material mat,
		VertexData vertData);

	void InternalCreate(Object obj, ShaderProgram shader, Material mat,
		VertexData vertData);

	template<class BufferObjTy>
	void FixInstances(render_data_t& dat, BufferObjTy& buf);

	void DrawBucket(render_data_t& dat);
};

MAKE_PERSIST_TRAITS(Render, Object, bool, ShaderProgram, Material, VertexData);

#endif
