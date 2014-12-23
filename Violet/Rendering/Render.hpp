#ifndef RENDER_H
#define RENDER_H

#include "stdafx.h"
#include "Shader.hpp"
#include "Object.hpp"
#include "VAO.hpp"
#include "Texture.hpp"
#include "Permavector.hpp"

#include <vector>
#include <memory>
#include <set>

#include <iostream>

class Render
{
public:
	class LocationProxy;
	//Maybe make mobility an option?
	LocationProxy Create(Object obj, ShaderProgram shader, UBO ubo, std::vector<Tex> texes,
		VertexData vertData, const Matrix4f& loc);
	void Destroy(Object obj);
	void draw() const;
	Matrix4f camera;

	Render();
	Render(const Render&) = delete;
	void operator=(const Render&) = delete;

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
	//UBO shared with all shaders
	ShaderProgram simpleShader;
	mutable UBO commonUBO;

	//our container of choice
	template<class... Args>
	using container = Permavector<Args...>;

	struct Shape
	{
		VAO vao;
		using InstanceVec = Permavector<Matrix4f, Eigen::aligned_allocator<Matrix4f>>;
		std::shared_ptr<InstanceVec> instances;
		void draw() const;

		Shape(std::tuple<VertexData, ShaderProgram> t)
			: vao(std::get<1>(t), std::get<0>(t))
			, instances(std::make_shared<InstanceVec>())
		{}

        Shape& operator=(Shape&&) = default;
		Shape(const Shape&) = delete;
		Shape(Shape&&); //MSVC sucks and can't default this

		bool operator==(const std::tuple<VertexData, ShaderProgram>& t)
		{ return vao == std::get<0>(t);	}
		bool operator!=(const std::tuple<VertexData, ShaderProgram>& t)
		{ return !(vao == std::get<0>(t)); }
	};

	struct Material
	{
		container<Shape> shapes;
		UBO materialProps;
		std::vector<Tex> textures;
		void draw() const;

		bool operator==(const std::tuple<UBO, std::vector<Tex>>& t)
		{
			return std::tie(materialProps, textures) == t;
		}
		bool operator!=(const std::tuple<UBO, std::vector<Tex>>& t)
		{
			return !(*this == t);
		}
		
        Material& operator=(Material&&) = default;

		Material(std::tuple<UBO, std::vector<Tex>> t)
			: materialProps(std::get<0>(t))
			, textures(std::get<1>(t))
		{}
		Material(const Material&) = delete;
		Material(Material&& other)
			: shapes(std::move(other.shapes))
			, materialProps(std::move(other.materialProps))
			, textures(std::move(other.textures))
		{}
	};

	struct Shader
	{
		ShaderProgram program;
		container<Material> materials;
		void draw() const;

		Shader(ShaderProgram program)
			: program(program) {}

		MEMBER_EQUALITY(ShaderProgram, program)

        Shader& operator=(Shader&&) = default;
		Shader(const Shader&) = delete;
		Shader(Shader&& other)
			: program(std::move(other.program)), materials(std::move(other.materials))
		{}
	};

	container<Shader> shaders;
	struct InstData
	{
		Matrix4f mat;
		InstData& operator=(const Matrix4f& m) { mat = m; return *this; }
		static const Schema schema;
	};

	mutable BufferObject<InstData, GL_ARRAY_BUFFER, GL_STREAM_DRAW> instanceBuffer;

public:
	//A thing that we can move the object with
	class LocationProxy
	{
	public:
		Matrix4f& operator*();
		Matrix4f* operator->() { return &**this; }
		LocationProxy(const LocationProxy& other) = default;
		LocationProxy(const LocationProxy&& other); //= default
	private:
		std::weak_ptr<Shape::InstanceVec> buf;
		Shape::InstanceVec::perma_ref obj;
		friend class Render;
		LocationProxy(std::weak_ptr<Shape::InstanceVec>, Shape::InstanceVec::perma_ref);
	};
	friend class LocationProxy;
};

#endif
