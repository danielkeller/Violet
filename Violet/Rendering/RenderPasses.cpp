#include "stdafx.h"
#include "RenderPasses.hpp"

#include "Render.hpp"
#include "Window.hpp"

RenderPasses::RenderPasses(Window& w, Render& r)
	: r(r), pickerPass(r, w)
	, screenShader("assets/screen")
	, screenMat(screenShader.MakeUBO("Material", "screenMat"))
	, screenQuad(screenShader, UnitBox)
{
	screenShader.TextureOrder({ "color", "picker" });
	WindowResize(w.dim.get());

	accessor<Eigen::Vector2i, RenderPasses*> acc = &RenderPasses::WindowResize;
	w.dim += make_magic(acc, this);
}

void RenderPasses::WindowResize(Eigen::Vector2i size)
{
	auto mainTex = TypedTex<>{ size };
	mainPass.AttachTex(mainTex);
	mainPass.AttachDepth(RenderBuffer{ GL_DEPTH_COMPONENT, size });
	mainPass.CheckStatus();

	auto pickerTex = pickerPass.WindowResize(size);

	screenMat.textures = {mainTex, pickerTex};
}

void RenderPasses::Draw()
{
	{
		auto bound = mainPass.Bind(GL_FRAMEBUFFER);
		mainPass.PreDraw();
		r.Draw();
	}

	pickerPass.Pick();
	
	screenShader.use();
	screenMat.use();
	screenQuad.Draw();
}

void RenderPasses::Highlight(Object o, Picker::Highlights type)
{
	screenMat.materialProps["selected"][type] = o.Id();
	screenMat.materialProps.Sync(); //TODO: this is redundant
}