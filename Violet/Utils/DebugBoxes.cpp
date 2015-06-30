#include "stdafx.h"
#include "DebugBoxes.hpp"

DebugBoxes::DebugBoxes(RenderPasses& passes)
    : enabled(false), mat("DebugBoxes", "assets/color")
    , vao(mat.Shader(), WireCube)
{
    vao.BindInstanceData(mat.Shader(), instBuffer);
    
    passes.CreateCustom(obj, [&](float)
    {
        if (enabled)
        {
            mat.use();
            vao.Draw();
        }
    });
}

void DebugBoxes::PushInst(Inst i)
{
    if (enabled)
        insts.emplace_back(std::move(i));
}

void DebugBoxes::PushVector(const Vector3f& from, const Vector3f& dir,
                                 const Vector3f& color)
{
    if (enabled)
    {
        Inst i;
        i.color = color;
        i.loc = Matrix4f::Zero();
        i.loc(3, 3) = 1;
        i.loc.block<3, 1>(0, 3) = from;
        i.loc.block<3, 1>(0, 0) = dir;
        PushInst(i);
    }
}

void DebugBoxes::Begin()
{
    if (enabled)
        insts.clear();
}

void DebugBoxes::End()
{
    if (enabled)
    {
        instBuffer.Data(insts);
        vao.NumInstances(static_cast<GLsizei>(insts.size()));
    }
}

template<>
const Schema AttribTraits<DebugBoxes::Inst>::schema = {
    AttribProperties{ "transform", GL_FLOAT, false, 0,                 { 4, 4 }, 4 * sizeof(float) },
    AttribProperties{ "color",     GL_FLOAT, false, 16 * sizeof(float),{ 3, 1 }, 0 },
};
