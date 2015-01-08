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
    : move(m.Create({}, CreateArrows(r))), m(m)
{}

void Tool::Update(Window& w, Object focused)
{
    if (!w.LeftMouse())
        return;
    
    int dir;
    
    if      (focused == x) dir = 0;
    else if (focused == y) dir = 1;
    else if (focused == z) dir = 2;
    else return;
    
    //transform the tool coordinate axes into screen space vectors
    Matrix4f screenAxes = w.PerspMat() * m.CameraMat() * move->ToMatrix();
    Eigen::Matrix<float, 2, 3> vecs = screenAxes.block<2, 3>(0, 0); //chop off z and w
    vecs.array().colwise() *= w.Dim().cast<float>().array() / 2.f; //scale into viewport pixel coordinates
    //how big is the object on the screen?
    float maxLen = vecs.colwise().norm().maxCoeff();
    //now scale each direction to that length
    vecs.colwise().normalize();
    vecs /= maxLen;
    
    move->pos[dir] += w.MouseDeltaPxl().dot(vecs.block<2, 1>(0, dir));
}