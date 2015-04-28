#include "stdafx.h"
#include "RenderPasses.hpp"

#include "Render.hpp"
#include "Window.hpp"

RenderPasses::RenderPasses(Window& w, Render& r)
	: r(r), w(w)
	, screenShader("assets/screen")
	, screenMat(screenShader.MakeUBO("Material", "screenMat"))
	, screenQuad(screenShader, UnitBox)
	, view({ Vector2i::Zero(), Vector2i{-1,-1} })
{
	screenShader.TextureOrder({ "color", "picker" });
}

void RenderPasses::WindowResize(Eigen::Vector2i size)
{
	if (size == view.size())
		return;

	auto mainTex = TypedTex<>{ size };
	auto pickerTex = TypedTex<std::uint32_t>{ size };

	fbo.AttachTexes({ mainTex, pickerTex });
	fbo.AttachDepth(RenderBuffer{ GL_DEPTH_COMPONENT, size });
	fbo.CheckStatus();

	screenMat.textures = { mainTex, pickerTex };
}

void RenderPasses::Draw(const Matrix4f& camera, Events e)
{
	WindowResize(e.view.size());
	view = e.view;

	//Draw the correct sides of things
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	r.camera = view.PerspMat() * camera;

	{
		auto bound = fbo.Bind(GL_FRAMEBUFFER);
		fbo.PreDraw({ Eigen::Matrix<GLuint, 4, 1>::Zero(), { Object::none.Id(), 0, 0, 0 } });
		r.Draw();
	}

	auto screenBox = view.screenBox;

	glViewport(screenBox.min().x(), e.dimVec.y() - screenBox.min().y() - screenBox.sizes().y(),
		screenBox.sizes().x(), screenBox.sizes().y());

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	
	screenShader.use();
	screenMat.use();
	screenQuad.Draw();
}

void RenderPasses::Highlight(Object o, Highlights type)
{
	screenMat.materialProps["selected"][type] = o.Id();
	screenMat.materialProps.Sync(); //TODO: this is redundant
}

Object RenderPasses::Pick(Vector2f posPixel) const
{
	auto posView = view.Pixel2View(posPixel).array();

	if ((posView < Eigen::Array2f::Zero()).any()
		|| (posView > Eigen::Array2f(1, 1)).any())
		return Object::invalid;

	return Object{ fbo.ReadPixel<std::uint32_t>(RenderPasses::PickerPass, posView) };
}