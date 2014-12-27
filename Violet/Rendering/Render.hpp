#ifndef RENDER_H
#define RENDER_H

#include "stdafx.h"
#include "Shader.hpp"
#include "Object.hpp"
#include "VAO.hpp"
#include "Texture.hpp"
#include "Containers/l_map.hpp"
#include "Containers/l_bag.hpp"

#include <vector>
#include <memory>

class Render
{
	struct InstData
	{
		Matrix4f mat;
        Object obj;
        InstData(const Matrix4f& m, Object o) : mat(m), obj(o) {}
		InstData& operator=(const Matrix4f& m) { mat = m; return *this; }
		static const Schema schema;
	};
    //todo: use l_map
    using InstanceVec = l_bag<InstData, Eigen::aligned_allocator<InstData>>;

public:
	class LocationProxy;
	//Maybe make mobility an option?
	LocationProxy Create(Object obj, ShaderProgram shader, UBO ubo, std::vector<Tex> texes,
		VertexData vertData, const Matrix4f& loc);
	void Destroy(Object obj);
	void Draw();
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
		std::weak_ptr<InstanceVec> buf;
		InstanceVec::perma_ref obj;
		friend class Render;
		LocationProxy(std::weak_ptr<InstanceVec>, InstanceVec::perma_ref);
	};

    enum Passes
    {
        PickerPass,
        NumPasses
    };

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
	//UBO shared with all shaders
	ShaderProgram simpleShader;
	mutable UBO commonUBO;

	//our container of choice
	template<class... Args>
	using container = l_bag<Args...>;

	struct Shape
	{
		VAO vao;
		std::shared_ptr<InstanceVec> instances;

		Shape(const VertexData& vertData, const ShaderProgram& program)
			: vao(program, vertData)
			, instances(std::make_shared<InstanceVec>())
		{}

        Shape& operator=(Shape&&) = default;
		Shape(const Shape&) = delete;
		Shape(Shape&& other) //MSVC sucks and can't default this
            : vao(std::move(other.vao))
            , instances(std::move(other.instances))
        {}

        MEMBER_EQUALITY(VertexData, vao)
	};

	struct Material
	{
		UBO materialProps;
		std::vector<Tex> textures;
        container<Shape>::perma_ref begin;

		bool operator==(const std::tuple<UBO, std::vector<Tex>>& t)
		{
			return std::tie(materialProps, textures) == t;
		}
		bool operator!=(const std::tuple<UBO, std::vector<Tex>>& t)
		{
			return !(*this == t);
		}
		
        Material& operator=(Material&&) = default;

		Material(UBO ubo, std::vector<Tex> texes, container<Shape>::perma_ref begin)
			: materialProps(ubo)
			, textures(texes)
            , begin(begin)
		{}
		Material(const Material&) = delete;
		Material(Material&& other)
			: materialProps(std::move(other.materialProps))
			, textures(std::move(other.textures))
			, begin(other.begin)
		{}
	};

	struct Shader
	{
		ShaderProgram program;
        container<Material>::perma_ref begin;

		Shader(ShaderProgram program, container<Material>::perma_ref begin)
			: program(program), begin(begin) {}

		MEMBER_EQUALITY(ShaderProgram, program)

        Shader& operator=(Shader&&) = default;
		Shader(const Shader&) = delete;
		Shader(Shader&& other)
			: program(std::move(other.program))
			, begin(other.begin)
		{}
	};

    container<Shader> shaders;
    container<Material> materials;
    container<Shape> shapes;

    template<class PerShader, class PerMaterial, class PerShape>
    void Iterate(PerShader psh, PerMaterial pm, PerShape ps);

	BufferObject<InstData, GL_ARRAY_BUFFER, GL_STREAM_DRAW> instanceBuffer;
};

#endif
