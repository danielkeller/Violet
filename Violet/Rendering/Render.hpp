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
	void Create(Object obj, std::array<ShaderProgram, AllPasses> shader,
		std::array<Material, AllPasses> mat, VertexData vertData,
		Mobilty mobile = Mobilty::No);
    //create with defaults
	void Create(Object obj, ShaderProgram shader, Material mat,
		VertexData vertData, Mobilty mobile = Mobilty::No);

	void Remove(Object obj);

	void Save(Object obj);
	void Load();

	void Draw();
    void DrawPass(int pass);
	Matrix4f camera;

	//FIXME: this is bad!
    void PassDefaults(Passes pass, ShaderProgram shader, Material mat);

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

    std::unordered_set<ShaderProgram> passShaders;
    std::unordered_set<Material> passMaterials;

	static const int ShaderLevel = 0, MatLevel = 1, VAOLevel = 2, InstanceLevel = 3;
	using render_data_t = tuple_tree<ShaderProgram, Material,
		Render_detail::Shape, Render_detail::InstData>;

	render_data_t staticRenderData;
	BufferObject<Render_detail::InstData, GL_ARRAY_BUFFER, GL_STATIC_DRAW> staticInstanceBuffer;

	std::unordered_map<Object, render_data_t::perma_refs_t> staticObjs;

	//maybe:
	//l_unordered_map<ShaderProgram, l_unordered_map<Material,
	//	l_unordered_map<VertexData, Render_detail::Shape>>> renderData;

	render_data_t renderData;
    BufferObject<Render_detail::InstData, GL_ARRAY_BUFFER, GL_STREAM_DRAW> instanceBuffer;

	std::unordered_map<Object, render_data_t::perma_refs_t> objs;

    std::array<std::unordered_set<ShaderProgram>::iterator, NumPasses> defaultShader;
	std::array<std::unordered_set<Material>::iterator, NumPasses> defaultMaterial;

	Render_detail::Shape&
		InternalCreateStatic(Object obj, ShaderProgram shader, Material mat,
		VertexData vertData);

	Render_detail::Shape&
		InternalCreate(Object obj, ShaderProgram shader, Material mat,
		VertexData vertData);

	template<class BufferObjTy>
	void FixInstances(render_data_t& dat, BufferObjTy& buf);

	void DrawBucket(render_data_t& dat);
	void DrawBucketPass(render_data_t& dat, int pass);
};

MAKE_PERSIST_TRAITS(Render, Object, bool, ShaderProgram, Material, VertexData);

#endif
