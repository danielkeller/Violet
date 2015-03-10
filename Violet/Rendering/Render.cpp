#include "stdafx.h"
#include "Render.hpp"

#include "Position.hpp"
#include "Mobile.hpp"

using namespace Render_detail;

Render::Render(Position& position, Mobile& m)
	: position(position), m(m)
	, simpleShader("assets/simple")
	, commonUBO(simpleShader.MakeUBO("Common", "Common"))
	, instanceBuffer()
{
	//Assumes no one binds over UBO::Common
	commonUBO.Bind();
}

void Render::PassDefaults(Passes pass, ShaderProgram shader, Material mat)
{
    defaultShader[pass] = passShaders.insert(shader).first;
    defaultMaterial[pass] = passMaterials.insert(mat).first;
}

//this really should be its own data structure
template<class data_t, class lower_data_t>
std::pair<typename lower_data_t::iterator, typename lower_data_t::iterator>
range_of(typename data_t::iterator it, data_t& data, lower_data_t& lower_data)
{
    if (it == data.end())
        return {lower_data.end(), lower_data.end()};
    else if (it + 1 == data.end())
        return {lower_data.find(it[0].begin), lower_data.end()};
    else
		return{ lower_data.find(it[0].begin), lower_data.find(it[1].begin) };
}

template<class BufferObjTy>
void Render::FixInstances(render_data_t& dat, BufferObjTy& buf)
{
	GLsizei offset = 0;

	for (auto& shader : dat)
		for (auto& mat : dat.children<MatLevel>(shader))
			for (auto& vao : dat.children<VAOLevel>(mat))
			{
				auto numInstances =
					static_cast<GLsizei>(dat.children<InstanceLevel>(vao).length());
				vao.first.vao.BindInstanceData(shader.first, buf, offset, numInstances);
				offset += numInstances;
			}
}

Shape& Render::InternalCreate(Object obj, ShaderProgram shader, Material mat, VertexData vertData)
{
	using InstPermaRef = render_data_t::perma_ref_t<InstanceLevel>;
	static accessor<Matrix4f, InstPermaRef> locaccesor = {
		[this](InstPermaRef ref) { return renderData.find<InstanceLevel>(ref)->mat; },
		[this](InstPermaRef ref, const Matrix4f& v)
		{
			renderData.find<InstanceLevel>(ref)->mat = v;
		}
	};

	auto refs = renderData.emplace(shader, mat, std::tie(shader, vertData),
		InstData{ obj, *m[obj] });

	auto instref = std::get<InstanceLevel>(refs);
	m[obj] = make_magic(locaccesor, instref);

	FixInstances(renderData, instanceBuffer);
	//send the actual data each draw call
	instanceBuffer.Data(renderData.get_level<InstanceLevel>().size());

	auto vaoref = std::get<VAOLevel>(refs);
	return renderData.get_level<VAOLevel>().find(vaoref)->first;
}

Shape& Render::InternalCreateStatic(Object obj, ShaderProgram shader,
	Material mat, VertexData vertData)
{
	using InstPermaRef = render_data_t::perma_ref_t<InstanceLevel>;
	static accessor<Transform, InstPermaRef> locaccesor = 
		[this](InstPermaRef ref, const Transform& v) mutable
		{
			auto& it = staticRenderData.find<InstanceLevel>(ref);
			it->mat = v.ToMatrix();
			size_t offset = it - staticRenderData.begin<InstanceLevel>();
			staticInstanceBuffer.Assign(offset, *it);
		};

	auto inst = InstData{ obj, position[obj].get().ToMatrix() };
	auto refs = staticRenderData.emplace(shader, mat, std::tie(shader, vertData), inst);

	auto instref = std::get<InstanceLevel>(refs);
	position[obj] += make_magic(locaccesor, instref);

	FixInstances(staticRenderData, staticInstanceBuffer);
	staticInstanceBuffer.Data(staticRenderData.get_level<InstanceLevel>().vector());

	auto vaoref = std::get<VAOLevel>(refs);
	return staticRenderData.get_level<VAOLevel>().find(vaoref)->first;
}

void Render::Create(Object obj, ShaderProgram shader, Material mat,
	VertexData vertData, Mobilty mobile)
{
	auto& shape = mobile == Mobilty::Yes
		? InternalCreate(obj, shader, mat, vertData)
		: InternalCreateStatic(obj, shader, mat, vertData);
	
    for (size_t i = 0; i < NumPasses; ++i)
    {
		shape.passShader[i] = defaultShader[i];
		shape.passMaterial[i] = defaultMaterial[i];
    }
}

void Render::Create(Object obj, std::array<ShaderProgram, AllPasses> shader,
	std::array<Material, AllPasses> mat, VertexData vertData, Mobilty mobile)
{
	auto& shape = mobile == Mobilty::Yes
		? InternalCreate(obj, shader[0], mat[0], vertData)
		: InternalCreateStatic(obj, shader[0], mat[0], vertData);
	
    for (size_t i = 0; i < NumPasses; ++i)
    {
		shape.passShader[i] = passShaders.insert(shader[i + 1]).first;
		shape.passMaterial[i] = passMaterials.insert(mat[i + 1]).first;
    }
}

void Render::DrawBucket(render_data_t& dat)
{
	for (auto& shader : dat)
	{
		shader.first.use();
		for (auto& mat : dat.children<MatLevel>(shader))
		{
			mat.first.use();
			for (auto& vao : dat.children<VAOLevel>(mat))
			{
				vao.first.vao.Draw();
			}
		}
	}
}

void Render::Draw()
{
	commonUBO["camera"] = camera;
	commonUBO.Sync();
	
	instanceBuffer.Data(renderData.get_level<InstanceLevel>().vector());

	DrawBucket(renderData);
	DrawBucket(staticRenderData);
}

void Render::DrawBucketPass(render_data_t& dat, int pass)
{
	auto curShader = passShaders.end();
	auto curMat = passMaterials.end();

	for (const auto& shape : dat.get_level<VAOLevel>())
	{
		if (curShader != shape.first.passShader[pass])
		{
			curShader = shape.first.passShader[pass];
			curShader->use();
		}
		if (curMat != shape.first.passMaterial[pass])
		{
			curMat = shape.first.passMaterial[pass];
			curMat->use();
		}
		shape.first.vao.Draw();
	}
}

void Render::DrawPass(int pass)
{
	commonUBO["camera"] = camera;
	commonUBO.Sync();

	DrawBucketPass(renderData, pass);
	DrawBucketPass(staticRenderData, pass);
}

template<>
const Schema AttribTraits<InstData>::schema = {
    { "transform", GL_FLOAT, false, 0, {4, 4}, 4 * sizeof(float) },
    { "object", GL_UNSIGNED_INT, true, 16 * sizeof(float), {1, 1}, 0 },
};
