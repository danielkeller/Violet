#include "stdafx.h"
#include "Tool.hpp"
#include "Rendering/Render.hpp"
#include "Geometry/Mesh.hpp"
#include "Window.hpp"

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
    ShaderProgram arrowPicker{"assets/tool_arrow_picker"};
    UBO xMat = arrowShader.MakeUBO("Material", "ToolX");
    xMat["direction"] = Vector3f{1,0,0};
    xMat.Sync();
    UBO yMat = arrowShader.MakeUBO("Material", "ToolY");
    yMat["direction"] = Vector3f{0,1,0};
    yMat.Sync();
    UBO zMat = arrowShader.MakeUBO("Material", "ToolZ");
    zMat["direction"] = Vector3f{0,0,1};
    zMat.Sync();
    auto xloc = r.Create(x, {arrowShader, arrowPicker}, {xMat, xMat}, arrow, Matrix4f::Identity());
    auto yloc = r.Create(y, {arrowShader, arrowPicker}, {yMat, yMat}, arrow, Matrix4f::Identity());
    auto zloc = r.Create(z, {arrowShader, arrowPicker}, {zMat, zMat}, arrow, Matrix4f::Identity());
    return {xloc, yloc, zloc};
}

Tool::Tool(Render& r, Mobile& m)
    : move(m.Create({}, CreateArrows(r)))
{}

void Tool::Update(Window& w, Object focused)
{
    if (!w.LeftMouse())
        return;
    
    float dist = (w.mouseDeltaPct().x() - w.mouseDeltaPct().y())*2.f;
    if (focused == x)
        move->pos.x() += dist;
    if (focused == y)
        move->pos.y() += dist;
    if (focused == z)
        move->pos.z() += dist;
}