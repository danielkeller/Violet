#ifndef RENDER_H
#define RENDER_H

#include "Shader.hpp"
#include "Object.hpp"
#include "VAO.hpp"
#include "Texture.hpp"
#include "Containers/l_bag.hpp"
#include <unordered_set>

#include <vector>
#include <memory>

enum Passes
{
    PickerPass,
    NumPasses,
    AllPasses
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

    HAS_HASH
};

MEMBER_HASH(Material, materialProps)

#include "Render_detail.hpp"

class Render
{
public:
	class LocationProxy;
	//Maybe make mobility an option?
	LocationProxy Create(Object obj, std::array<ShaderProgram, AllPasses> shader,
        std::array<Material, AllPasses> mat, VertexData vertData, const Matrix4f& loc);
	void Destroy(Object obj);
	void Draw();
    void DrawPass(int pass);
	Matrix4f camera;

	Render();
	Render(const Render&) = delete;
	void operator=(const Render&) = delete;

	//A thing that we can move the object with
	class LocationProxy
	{
	public:
		Matrix4f& operator*();
		Matrix4f* operator->() { return &**this; }
		LocationProxy(const LocationProxy& other) = default;
		LocationProxy(const LocationProxy&& other); //= default
	private:
		std::weak_ptr<Render_detail::InstanceVec> buf;
		Render_detail::InstanceVec::perma_ref obj;
		friend class Render;
		LocationProxy(std::weak_ptr<Render_detail::InstanceVec>, Render_detail::InstanceVec::perma_ref);
	};

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
	//UBO shared with all shaders
	ShaderProgram simpleShader;
	mutable UBO commonUBO;

    std::unordered_set<ShaderProgram> passShaders;
    std::unordered_set<Material> passMaterials;

    l_bag<Render_detail::Shader> shaders;
    l_bag<Render_detail::T_Material> materials;
    l_bag<Render_detail::Shape> shapes;
    //or, unord_l_map<Shader, unord_l_map<T_Material, unord_l_set<VAO>>>/
    //const insert but fragmented data

    template<class PerShader, class PerMaterial, class PerShape>
    void Iterate(PerShader psh, PerMaterial pm, PerShape ps);

	BufferObject<Render_detail::InstData, GL_ARRAY_BUFFER, GL_STREAM_DRAW> instanceBuffer;
};

#endif
