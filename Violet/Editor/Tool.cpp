#include "stdafx.h"
#include "Tool.hpp"
#include "Rendering/Render.hpp"
#include "Geometry/Mesh.hpp"
#include "Picker.hpp"
#include "Time.hpp"

//Arrow towards +x
std::vector<Vector3f> arrowVerts = {
    {0, .1, 0}, {0, -.1, 0},
    {1, .1, 0}, {1, -.1, 0},
    {1, .5, 0}, {1, -.5, 0}, {1.5, 0, 0}
};

std::vector<TriInd> arrowInds = {
    {0, 1, 2},
    {1, 3, 2},
    {4, 5, 6}
};

std::vector<Render::LocationProxy> Tool::CreateArrows(Render& r)
{
    VertexData arrow("ToolArrow", arrowVerts, arrowInds);
    
    ShaderProgram arrowShader{"assets/tool_arrow"};
    UBO xMat = arrowShader.MakeUBO("Material", "ToolX");
    xMat["direction"] = Vector3f{1,0,0};
    xMat.Sync();
    UBO yMat = arrowShader.MakeUBO("Material", "ToolY");
    yMat["direction"] = Vector3f{0,1,0};
    yMat.Sync();
    UBO zMat = arrowShader.MakeUBO("Material", "ToolZ");
    zMat["direction"] = Vector3f{0,0,1};
    zMat.Sync();
    auto xloc = r.Create(x, {arrowShader, {}}, {xMat, {}}, arrow, Matrix4f::Identity());
    auto yloc = r.Create(y, {arrowShader, {}}, {yMat, {}}, arrow, Matrix4f::Identity());
    auto zloc = r.Create(z, {arrowShader, {}}, {zMat, {}}, arrow, Matrix4f::Identity());
    return {xloc, yloc, zloc};
}

Tool::Tool(Render& r, Picker& pick, Mobile& m)
    : move(m.Create({}, CreateArrows(r)))
{}

void Tool::Update(Time& t)
{
    //move->pos.x() = std::sin(t.SimTimeMs()*.001);
}