#include "stdafx.h"
#include "Box.hpp"

#include "Rendering/Shader.hpp"
#include "Rendering/VAO.hpp"
#include "Window.hpp"
#include "Layout.hpp"

void DrawBox(const Layout& l)
{
	DrawBox(l.pos, l.size);
}

void DrawBox(Vector2i corner, Vector2i size)
{
	static ShaderProgram boxShdr("assets/uibox");
	static UBO boxMat = boxShdr.MakeUBO("Material", "BoxMat");
	static VAO boxVAO(boxShdr, UnitBox);

	boxMat["top"] = corner;
	boxMat["size"] = size;
	boxMat.Sync();

	boxShdr.use();
	boxMat.Bind();
	boxVAO.Draw();
}

void WinResize(Vector2i sz)
{
	static ShaderProgram boxShdr("assets/uibox");
	static UBO boxMat = boxShdr.MakeUBO("Material", "BoxMat");
	boxMat["viewport"] = sz;
}

void BoxInit(Window& w)
{
	w.dim += make_magic(accessor<Vector2i>(&WinResize));
	WinResize(*w.dim);
}