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
	std::weak_ptr<InstanceVec> buf, InstanceVec::perma_ref obj)
	: buf(buf), obj(obj)
{}

Render::LocationProxy::LocationProxy(const LocationProxy&& other)
	: buf(other.buf), obj(other.obj)
{}

Matrix4f& Render::LocationProxy::operator*()
{
	return buf.lock()->get(obj)->mat;
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

Render::LocationProxy Render::Create(Object obj, std::array<ShaderProgram, AllPasses> shader,
    std::array<Material, AllPasses> mat, VertexData vertData, const Matrix4f& loc)
{
    auto shaderit = std::find(shaders.begin(), shaders.end(), shader[0]);
    //special case: shaderit is, or will be, the last one
    auto matend = shaderit + 1 >= shaders.end() ? materials.get_perma(materials.end()) : shaderit[1].begin;
    if (shaderit == shaders.end())
    {
        //material range is empty
        shaderit = shaders.get(shaders.emplace_back(shader[0], matend));
    } //shaderit is now valid

    auto matbegin = materials.get(shaderit->begin);
    auto materialit = std::find(matbegin, materials.get(matend), mat[0]);
    //special case: materialit is, or will be, the last one
    auto shapeend = materialit + 1 >= materials.end() ? shapes.get_perma(shapes.end()) : materialit[1].begin;
    if (materialit == materials.get(matend))
    {
        //put it at the beginning of the shader's range
        shaderit->begin = materials.emplace(matbegin, mat[0], shapeend);
        materialit = materials.get(shaderit->begin);
    } //materialit is now valid

    auto shapebegin = shapes.get(materialit->begin);
    auto shapeit = std::find(shapebegin, shapes.get(shapeend), vertData);
    if (shapeit == shapes.get(shapeend))
    {
        //put it at the beginning of the material's range
        materialit->begin = shapes.emplace(shapebegin, vertData, shader[0]);
        shapeit = shapes.get(materialit->begin);
    } //shapeit is now valid

	auto objRef = shapeit->instances->emplace_back(loc, obj);

    for (size_t i = 0; i < NumPasses; ++i)
    {
        shapeit->passShader[i] = passShaders.insert(shader[i+1]).first;
        shapeit->passMaterial[i] = passMaterials.insert(mat[i+1]).first;
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
        shape.vao.BindInstanceData(curShader->program, instanceBuffer, offset,
            static_cast<GLsizei>(shape.instances->size()));
        offset += shape.instances->size();
    });

	instanceBuffer.Data(offset);

	return LocationProxy{ shapeit->instances, objRef };
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
		auto mapping = instanceBuffer.Map(GL_WRITE_ONLY);
		auto mapit = mapping.begin();
        for (auto& shape : shapes)
            mapit = std::copy(shape.instances->begin(), shape.instances->end(), mapit);
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

template<>
const Schema AttribTraits<InstData>::schema = {
    { "transform", GL_FLOAT, false, 0, {4, 4}, 4 * sizeof(float) },
    { "object", GL_UNSIGNED_INT, true, 16 * sizeof(float), {1, 1}, 0 },
};
