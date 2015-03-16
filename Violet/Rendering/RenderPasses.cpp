#include "stdafx.h"
#include "RenderPasses.hpp"

#include "Render.hpp"
#include "Window.hpp"

RenderPasses::RenderPasses(Window& w, Render& r)
	: r(r)
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
	auto pickerTex = TypedTex<std::uint32_t>{ size };

	fbo.AttachTexes({ mainTex, pickerTex });
	fbo.AttachDepth(RenderBuffer{ GL_DEPTH_COMPONENT, size });
	fbo.CheckStatus();

	screenMat.textures = {mainTex, pickerTex};
}

void RenderPasses::Draw()
{
	{
		auto bound = fbo.Bind(GL_FRAMEBUFFER);
		fbo.PreDraw({ Eigen::Matrix<GLuint, 4, 1>::Zero(), { Object::none.Id(), 0, 0, 0 } });
		r.Draw();
	}
	
	screenShader.use();
	screenMat.use();
	screenQuad.Draw();
}

void RenderPasses::Highlight(Object o, Highlights type)
{
	screenMat.materialProps["selected"][type] = o.Id();
	screenMat.materialProps.Sync(); //TODO: this is redundant
}

Object RenderPasses::Pick(Vector2f posView) const
{
	return Object{ fbo.ReadPixel<std::uint32_t>(RenderPasses::PickerPass, posView) };
}