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

template<>
const Schema AttribTraits<Eigen::AlignedBox2i>::schema = {
	{ "minBox", GL_INT, true, 0,               { 2, 1 } },
	{ "maxBox", GL_INT, true, 2 * sizeof(int), { 2, 1 } },
};

void DrawBox(Vector2i corner, Vector2i size)
{
	static ShaderProgram boxShdr("assets/uibox");
	static VAO boxVAO(boxShdr, UnitBox);
	static BufferObject<Eigen::AlignedBox2i, GL_ARRAY_BUFFER, GL_STREAM_DRAW> boxInstances(1);

	boxInstances.Assign(0, { corner, corner + size });

	boxVAO.BindInstanceData(boxShdr, boxInstances);

	boxShdr.use();
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