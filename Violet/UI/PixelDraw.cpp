#include "stdafx.h"
#include "PixelDraw.hpp"

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
	BindPixelUBO();
	boxVAO.Draw();
}

static void WinResize(Vector2i sz)
{
	static ShaderProgram txtShdr{ "assets/uibox" };
	static UBO pixelUBO = txtShdr.MakeUBO("Common", "PixelCommon");
	pixelUBO["pixelMat"] = PixelMat(sz);
	pixelUBO.Sync();
}

void PixelInit(Window& w)
{
	w.dim += make_magic(accessor<Vector2i>(&WinResize));
	WinResize(*w.dim);
}

void BindPixelUBO()
{
	static ShaderProgram pxlShdr{ "assets/uibox" };
	static UBO pixelUBO = pxlShdr.MakeUBO("Common", "PixelCommon");
	pixelUBO.Bind();
}