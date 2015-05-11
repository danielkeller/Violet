#include "stdafx.h"
#include "Tool.hpp"
#include "Rendering/Render.hpp"
#include "Geometry/Mesh.hpp"
#include "Window.hpp"

//Arrow towards +x
std::vector<Vector3f> arrowVerts = {
    {0.f, .1f, 0.f}, {0.f, -.1f, 0.f},
    {1.f, .1f, 0.f}, {1.f, -.1f, 0.f},
    {1.f, .5f, 0.f}, {1.f, -.5f, 0.f}, {1.5f, 0.f, 0.f}
};

std::vector<TriInd> arrowInds = {
    {0, 1, 2},
    {1, 3, 2},
    {4, 5, 6}
};

Tool::Tool(Render& r, Position& position)
	: position(position), move(position[x] + position[y] + position[z])
{
	VertexData arrow("ToolArrow", arrowVerts, arrowInds);

	ShaderProgram arrowShader{ "assets/tool_arrow" };
	UBO xMat = arrowShader.MakeUBO("Material", "ToolX");
	xMat["direction"] = Vector3f{ 1, 0, 0 }; //Vector3f::UnitX();
	UBO yMat = arrowShader.MakeUBO("Material", "ToolY");
	yMat["direction"] = Vector3f{ 0, 1, 0 };
	UBO zMat = arrowShader.MakeUBO("Material", "ToolZ");
	zMat["direction"] = Vector3f{ 0, 0, 1 };
	r.Create(x, arrowShader, xMat, arrow, Mobilty::Yes);
	r.Create(y, arrowShader, yMat, arrow, Mobilty::Yes);
	r.Create(z, arrowShader, zMat, arrow, Mobilty::Yes);
}

void Tool::SetTarget(magic_ptr<Transform> t)
{
	target = t;
	if (t)
		move->pos = t.get().pos; //snap over to moved object
	else
		move->pos = Vector3f::Zero(); //better, hide the tool
}

void Tool::Update(Events& e, Object camera, Object focused)
{
	if (!e.MouseButton(GLFW_MOUSE_BUTTON_LEFT))
        return;
	if (!target)
		return;

    int dir;
    
    if      (focused == x) dir = 0;
    else if (focused == y) dir = 1;
    else if (focused == z) dir = 2;
    else return;

	float delta = e.view.ApparentPos(e.MousePosPxl(),
		position[camera]->ToMatrix() * move->ToMatrix())[dir];
	move->pos[dir] += delta;
	target->pos[dir] += delta;

	e.PopMouse();
}