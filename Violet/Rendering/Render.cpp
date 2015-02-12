#include "stdafx.h"
#include "Render.hpp"
#include <algorithm>

using namespace Render_detail;

Render::Render()
	: simpleShader("assets/simple")
	, commonUBO(simpleShader.MakeUBO("Common", "Common"))
	, instanceBuffer()
{
	//Assumes no one binds over UBO::Common
	commonUBO.Bind();
}

Render::LocationProxy::LocationProxy(
	InstanceVec* buf, InstanceVec::perma_ref obj)
	: buf(buf), obj(obj)
{}

Render::LocationProxy::LocationProxy(LocationProxy&& other)
	: buf(other.buf), obj(other.obj)
{}

Matrix4f& Render::LocationProxy::operator*()
{
	return buf->get(obj)->mat;
}

template<class PerShader, class PerMaterial, class PerShape>
void Render::Iterate(PerShader psh, PerMaterial pm, PerShape ps)
{
    auto materialit = materials.begin();
    auto shapeit = shapes.begin();
    for (auto shaderit = shaders.begin(); shaderit != shaders.end(); ++shaderit)
    {
        psh(*shaderit);
        auto nextshader = shaderit + 1 == shaders.end() ? materials.end() : materials.get(shaderit[1].begin);
        for(; materialit != nextshader; ++materialit)
        {
            pm(*materialit);
            auto nextmaterial = materialit + 1 == materials.end() ? shapes.end() : shapes.get(materialit[1].begin);
            for (; shapeit != nextmaterial; ++shapeit)
            {
                ps(*shapeit);
            }
        }
    }
}

void Render::PassDefaults(Passes pass, ShaderProgram shader, Material mat)
{
    defaultShader[pass] = passShaders.insert(shader).first;
    defaultMaterial[pass] = passMaterials.insert(mat).first;
}

template<class data_t, class lower_data_t>
std::pair<typename lower_data_t::iterator, typename lower_data_t::iterator>
range_of(typename data_t::iterator it, data_t& data, lower_data_t& lower_data)
{
    if (it == data.end())
        return {lower_data.end(), lower_data.end()};
    else if (it + 1 == data.end())
        return {lower_data.get(it[0].begin), lower_data.end()};
    else
        return {lower_data.get(it[0].begin), lower_data.get(it[1].begin)};
}

std::pair<l_bag<Shape>::iterator, Render::LocationProxy>
Render::InternalCreate(Object obj, ShaderProgram shader, Material mat,
    VertexData vertData, const Matrix4f& loc)
{
    auto shaderit = std::find(shaders.begin(), shaders.end(), shader);
    auto matrange = range_of(shaderit, shaders, materials);
    auto matit = std::find(matrange.first, matrange.second, mat);
    auto shaperange = range_of(matit, materials, shapes);
    auto shapeit = std::find(shaperange.first, shaperange.second, vertData);
    auto instrange = range_of(shapeit, shapes, instances);
	//This call has issues with emplacing normally, because of alignment
    auto instref = instances.emplace(instrange.second, InstData(loc, obj));
    
    auto shaperef = shapes.get_perma(shapes.end());
    if (shapeit == shaperange.second)
    {
        shaperef = shapes.emplace(shapeit, vertData, shader, instref);
        shapeit = shapes.get(shaperef);
    }
    
    if (shaderit == shaders.end())
    {
        shaders.emplace(shaderit, shader,
            materials.emplace(matit, mat, shaperef));
    }
    else if (matit == matrange.second)
    {
        materials.emplace(matit, mat, shaperef);
    }

	GLsizei offset = 0;
    Shader* curShader;
    Iterate(
    [&](Shader& shader)
    {
        curShader = &shader;
    },
    [](T_Material& m) {},
    [&](Shape& shape)
    {
        auto instend = &shape == &shapes.back() ? instances.end() : instances.get((&shape)[1].begin);
		auto numInstances = static_cast<GLsizei>(instend - instances.get(shape.begin));
        shape.vao.BindInstanceData(curShader->program, instanceBuffer, offset, numInstances);
        offset += numInstances;
    });

    instances.resize(offset);
	instanceBuffer.Data(offset);

	return {shapeit, {&instances, instref}};
}

Render::LocationProxy Render::Create(Object obj, ShaderProgram shader, Material mat,
    VertexData vertData, const Matrix4f& loc)
{
    auto p = InternalCreate(obj, shader, mat, vertData, loc);

    for (size_t i = 0; i < NumPasses; ++i)
    {
        p.first->passShader[i] = defaultShader[i];
        p.first->passMaterial[i] = defaultMaterial[i];
    }

    return p.second;
}

Render::LocationProxy Render::Create(Object obj, std::array<ShaderProgram, AllPasses> shader,
    std::array<Material, AllPasses> mat, VertexData vertData, const Matrix4f& loc)
{
    auto p = InternalCreate(obj, shader[0], mat[0], vertData, loc);

    for (size_t i = 0; i < NumPasses; ++i)
    {
        p.first->passShader[i] = passShaders.insert(shader[i+1]).first;
        p.first->passMaterial[i] = passMaterials.insert(mat[i+1]).first;
    }

    return p.second;
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

	{
		//auto mapping = instanceBuffer.Map(GL_WRITE_ONLY);
        //auto mapit = instances.begin(); //mapping.begin();
        //for (auto& shape : shapes)
         //   mapit = std::copy(shape.instances->begin(), shape.instances->end(), mapit);
        instanceBuffer.Data(instances.vector());
	} //unmap
    
    Iterate(
    [](Shader& shader)
    {
        shader.program.use();
    },
    [](T_Material& material)
    {
        material.mat.use();
    },
    [](Shape& shape)
    {
        shape.vao.Draw();
    });

}

void Render::DrawPass(int pass)
{
	commonUBO["camera"] = camera;
	commonUBO.Sync();

    auto curShader = passShaders.end();
    auto curMat = passMaterials.end();

    for (const auto& shape : shapes)
    {
        if (curShader != shape.passShader[pass])
        {
            curShader = shape.passShader[pass];
            curShader->use();
        }
        if (curMat != shape.passMaterial[pass])
        {
            curMat = shape.passMaterial[pass];
            curMat->use();
        }
        shape.vao.Draw();
    }
}

Render::LocationProxy Render::GetLocProxyFor(Object obj)
{
    auto inst = std::find(instances.begin(), instances.end(), obj);
    if (inst == instances.end())
        throw std::runtime_error("No such object " + to_string(obj));
    return {&instances, instances.get_perma(inst)};
}

template<>
const Schema AttribTraits<InstData>::schema = {
    { "transform", GL_FLOAT, false, 0, {4, 4}, 4 * sizeof(float) },
    { "object", GL_UNSIGNED_INT, true, 16 * sizeof(float), {1, 1}, 0 },
};
