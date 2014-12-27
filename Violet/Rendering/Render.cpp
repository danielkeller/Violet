#include "stdafx.h"
#include "Render.hpp"
#include <algorithm>

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

Render::LocationProxy Render::Create(Object obj, ShaderProgram shader, UBO ubo,
	std::vector<Tex> texes, VertexData vertData, const Matrix4f& loc)
{
    auto shaderit = std::find(shaders.begin(), shaders.end(), shader);
    //special case: shaderit is, or will be, the last one
    auto matend = shaderit + 1 >= shaders.end() ? materials.get_perma(materials.end()) : shaderit[1].begin;
    if (shaderit == shaders.end())
    {
        //material range is empty
        shaderit = shaders.get(shaders.emplace_back(shader, matend));
    } //shaderit is now valid

    auto matbegin = materials.get(shaderit->begin);
    auto materialit = std::find(matbegin, materials.get(matend), std::make_tuple(ubo, texes));
    //special case: materialit is, or will be, the last one
    auto shapeend = materialit + 1 >= materials.end() ? shapes.get_perma(shapes.end()) : materialit[1].begin;
    if (materialit == materials.get(matend))
    {
        //put it at the beginning of the shader's range
        shaderit->begin = materials.emplace(matbegin, ubo, texes, shapeend);
        materialit = materials.get(shaderit->begin);
    } //materialit is now valid

    auto shapebegin = shapes.get(materialit->begin);
    auto shapeit = std::find(shapebegin, shapes.get(shapeend), vertData);
    if (shapeit == shapes.get(shapeend))
    {
        //put it at the beginning of the material's range
        materialit->begin = shapes.emplace(shapebegin, vertData, shader);
        shapeit = shapes.get(materialit->begin);
    } //shapeit is now valid

	auto objRef = shapeit->instances->emplace_back(loc, obj);

	size_t offset = 0;
    Shader* curShader;
    Iterate(
    [&](Shader& shader)
    {
        curShader = &shader;
    },
    [](Material& m) {},
    [&](Shape& shape)
    {
        shape.vao.BindInstanceData(curShader->program, instanceBuffer, offset,
            shape.instances->size());
        offset += shape.instances->size();
    });

	instanceBuffer.Data(offset);

	return LocationProxy{ shapeit->instances, objRef };
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
    [](Material& material)
    {
        for (GLuint i = 0; i < material.textures.size(); ++i)
            material.textures[i].Bind(i);

        material.materialProps.Bind();
    },
    [](Shape& shape)
    {
        shape.vao.Draw();
    });

}

const Schema Render::InstData::schema = {
	{ "transform", 4, GL_FLOAT, 0, 4, 4 * sizeof(float) },
	{ "object", 1, GL_UNSIGNED_INT, 16 * sizeof(float), 1, 0 },
};
