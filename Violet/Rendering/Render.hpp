#ifndef RENDER_H
#define RENDER_H

#include "stdafx.h"
#include "Shader.hpp"
#include "Object.hpp"
#include "Uniform.hpp"
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
		VAO vao, const Matrix4f& loc);
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
		void draw() const;

		Shape(VAO vao)
			: vao(vao) {}

		Shape(const Shape&) = delete;
		Shape(Shape&&); //MSVC sucks and can't default this

		bool operator==(const VAO& other)
			{ return vao == other; }
		bool operator!=(const VAO& other)
			{ return !(vao == other); }
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

		bool operator==(const ShaderProgram& other)
			{ return program == other; }
		bool operator!=(const ShaderProgram& other)
			{ return !(program == other); }

		Shader(const Shader&) = delete;
		Shader(Shader&& other)
			: program(std::move(other.program)), materials(std::move(other.materials))
		{}
	};

	container<Shader> shaders;
	mutable std::set<container<Shape>::perma_ref> dirty;

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
		std::shared_ptr<VAO::InstanceBuf> buf;
		VAO::InstanceBuf::perma_ref obj;
		friend class Render;
		LocationProxy(std::shared_ptr<VAO::InstanceBuf>, VAO::InstanceBuf::perma_ref);
	};
	friend class LocationProxy;
};

#endif
