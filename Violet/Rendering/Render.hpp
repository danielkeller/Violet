#ifndef RENDER_H
#define RENDER_H

#include "Shader.hpp"
#include "Object.hpp"
#include "VAO.hpp"
#include "Texture.hpp"
#include "Containers/l_bag.hpp"
#include "Containers/l_unordered_map.hpp"
#include "Containers/tuple_tree.hpp"
#include <unordered_set>

#include <vector>
#include <memory>
#include <array>

class Position;
class Mobile;

enum Passes
{
    PickerPass,
    NumPasses,
    AllPasses
};

enum class Mobilty
{
	Yes,
	No
};

struct Material
{
    UBO materialProps;
    std::vector<Tex> textures;

    bool operator==(const Material& t) const
    {
        return materialProps == t.materialProps && textures == t.textures;
    }
    bool operator!=(const Material& t) const
    {
        return !(*this == t);
    }
    void use() const;
    
    Material() = default;
    Material(const UBO& props) : materialProps(props) {}
    Material(const UBO& props, const std::vector<Tex>& texs)
        : materialProps(props) , textures(texs){}

    HAS_HASH
};

MEMBER_HASH(Material, materialProps)

#include "Render_detail.hpp"

class Render
{
public:
	void Create(Object obj, std::array<ShaderProgram, AllPasses> shader,
		std::array<Material, AllPasses> mat, VertexData vertData,
		Mobilty mobile = Mobilty::No);
    //create with defaults
	void Create(Object obj, ShaderProgram shader, Material mat,
		VertexData vertData,
		Mobilty mobile = Mobilty::No);

	void Destroy(Object obj);

	void Draw();
    void DrawPass(int pass);
	Matrix4f camera;

    void PassDefaults(Passes pass, ShaderProgram shader, Material mat);

	Render(Position&, Mobile&);
	Render(const Render&) = delete;
	void operator=(const Render&) = delete;

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
	Position& position;
	Mobile& m;

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

	//maybe:
	//l_unordered_map<ShaderProgram, l_unordered_map<Material,
	//	l_unordered_map<VertexData, Render_detail::Shape>>> renderData;

	render_data_t renderData;
    BufferObject<Render_detail::InstData, GL_ARRAY_BUFFER, GL_STREAM_DRAW> instanceBuffer;

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

#endif
