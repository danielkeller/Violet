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
/*
template<class PerShader, class PerMaterial, class PerShape>
void Render::IterateStatic(PerShader psh, PerMaterial pm, PerShape ps)
{
    auto materialit = materials.begin();
    auto shapeit = shapes.begin();
    for (auto shaderit = shaders.begin(); shaderit != shaders.end(); ++shaderit)
    {
        psh(*shaderit);
        auto nextshader = shaderit + 1 == shaders.end() ? materials.end() : materials.find(shaderit[1].begin);
        for(; materialit != nextshader; ++materialit)
        {
            pm(*materialit);
			auto nextmaterial = materialit + 1 == materials.end() ? shapes.end() : shapes.find(materialit[1].begin);
            for (; shapeit != nextmaterial; ++shapeit)
            {
                ps(*shapeit);
            }
        }
    }
}*/

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

/*
This bug needs to be submitted to MS when their website starts working

struct test
{
	using tup = std::tuple<std::vector<int>, std::vector<char>, std::vector<float>>;

	template<int pos>
	using tup_elem = typename std::tuple_element<pos, tup>::type;

	template<int pos>
	using tup_elem_elem = typename tup_elem<pos>::value_type;

	template<int pos>
	void bar(tup_elem_elem<pos> val)
	{}
};

void foo()
{
	test t;
	t.bar<1>('c');
}
*/

Render::static_render_t::iter_t<Render::VAOLevel>
Render::InternalCreate(Object obj, ShaderProgram shader, Material mat,
    VertexData vertData)
{
	using InstPermaRef = static_render_t::perma_ref_t<InstanceLevel>;
	accessor<Matrix4f, InstPermaRef> locaccesor = {
		[this](InstPermaRef ref) { return staticRenderData.find<InstanceLevel>(ref)->mat;
		},
		[this](InstPermaRef ref, const Matrix4f& v)
		{
			staticRenderData.find<InstanceLevel>(ref)->mat = v;
		}
	};

	auto refs = staticRenderData.emplace(shader, mat, std::tie(shader, vertData), obj);
	auto instref = std::get<InstanceLevel>(refs);
	m[obj] = make_magic(locaccesor, instref);

	GLsizei offset = 0;
	
	for (auto& shader : staticRenderData)
		for (auto& mat : staticRenderData.children<MatLevel>(shader))
			for (auto& vao : staticRenderData.children<VAOLevel>(mat))
			{
				auto numInstances = 
					static_cast<GLsizei>(staticRenderData.children<InstanceLevel>(vao).length());
				vao.first.vao.BindInstanceData(shader.first, instanceBuffer, offset, numInstances);
				offset += numInstances;
			}
	instanceBuffer.Data(offset);

	auto vaoref = std::get<VAOLevel>(refs);
	auto& vaolevel = staticRenderData.get_level<VAOLevel>();
	return vaolevel.find(vaoref);
}

void Render::Create(Object obj, ShaderProgram shader, Material mat,
	VertexData vertData, Mobilty mobile)
{
    auto it = InternalCreate(obj, shader, mat, vertData);
	
    for (size_t i = 0; i < NumPasses; ++i)
    {
        it->first.passShader[i] = defaultShader[i];
		it->first.passMaterial[i] = defaultMaterial[i];
    }
}

void Render::Create(Object obj, std::array<ShaderProgram, AllPasses> shader,
	std::array<Material, AllPasses> mat, VertexData vertData, Mobilty mobile)
{
    auto it = InternalCreate(obj, shader[0], mat[0], vertData);
	
    for (size_t i = 0; i < NumPasses; ++i)
    {
		it->first.passShader[i] = passShaders.insert(shader[i + 1]).first;
		it->first.passMaterial[i] = passMaterials.insert(mat[i + 1]).first;
    }
}

void Material::use() const
{
    for (GLuint i = 0; i < textures.size(); ++i)
        textures[i].Bind(i);

    materialProps.Bind();
}

void Render::Draw()
{
	commonUBO["camera"] = camera;
	commonUBO.Sync();
	
	instanceBuffer.Data(staticRenderData.get_level<InstanceLevel>().vector());
	for (auto& shader : staticRenderData)
	{
		shader.first.use();
		for (auto& mat : staticRenderData.children<MatLevel>(shader))
		{
			mat.first.use();
			for (auto& vao : staticRenderData.children<VAOLevel>(mat))
			{
				vao.first.vao.Draw();
			}
		}
	}
}

void Render::DrawPass(int pass)
{
	commonUBO["camera"] = camera;
	commonUBO.Sync();

    auto curShader = passShaders.end();
    auto curMat = passMaterials.end();
	
    for (const auto& shape : staticRenderData.get_level<VAOLevel>())
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

template<>
const Schema AttribTraits<InstData>::schema = {
    { "transform", GL_FLOAT, false, 0, {4, 4}, 4 * sizeof(float) },
    { "object", GL_UNSIGNED_INT, true, 16 * sizeof(float), {1, 1}, 0 },
};
