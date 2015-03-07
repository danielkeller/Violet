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
	//TODO: make mobility an option
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
	/*
    l_bag<Render_detail::Shader> shaders;
    l_bag<Render_detail::T_Material> materials;
	l_bag<Render_detail::Shape> shapes;*/
	static const int ShaderLevel = 0, MatLevel = 1, VAOLevel = 2, InstanceLevel = 3;
	using static_render_t = tuple_tree<ShaderProgram, Material,
		Render_detail::Shape, Render_detail::InstData>;
	static_render_t staticRenderData;
	BufferObject<Render_detail::InstData, GL_ARRAY_BUFFER, GL_STATIC_DRAW> staticInstanceBuffer;

	//l_unordered_map<ShaderProgram, l_unordered_map<Material,
	//	l_unordered_map<VertexData, Render_detail::Shape>>> dynamicShapes;
    //Render_detail::InstanceVec instances;
    BufferObject<Render_detail::InstData, GL_ARRAY_BUFFER, GL_STREAM_DRAW> instanceBuffer;

    std::array<std::unordered_set<ShaderProgram>::iterator, NumPasses> defaultShader;
	std::array<std::unordered_set<Material>::iterator, NumPasses> defaultMaterial;

	static_render_t::iter_t<VAOLevel>
		InternalCreate(Object obj, ShaderProgram shader, Material mat,
		VertexData vertData);
};

#endif
