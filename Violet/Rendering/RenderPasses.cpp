#include "stdafx.h"
#include "RenderPasses.hpp"

#include "Render.hpp"
#include "Window.hpp"

#include "Utils/Math.hpp"

RenderPasses::RenderPasses(Position& p, Window& w, Render& r)
	: r(r), w(w), mobile(p), camera(Object::invalid)
	, simpleShader("assets/simple")
	, commonUBO(simpleShader, "Common")
	, screenMat("screenMat", { "assets/screen" })
	, screenQuad(screenMat.Shader(), UnitBox)
	, view({ Vector2i::Zero(), Vector2i{-1,-1} })
{
	screenMat.Shader().TextureOrder({ "color", "picker" });
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

	screenMat.Textures() = { mainTex, pickerTex };
}

void RenderPasses::Camera(Object c)
{
	camera = c;
}

void RenderPasses::CreateCustom(Object obj, Custom draw)
{
	customs[obj] = draw;
}

void RenderPasses::Draw(Events e, float alpha)
{
	//just don't bother
	if ((e.view.size().array() == Eigen::Array2i::Zero()).any())
		return;

	WindowResize(e.view.size());
	view = e.view;

	//Draw the correct sides of things
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	//for now it's just this
	InstData cameraMat(camera);
	mobile.Update(alpha, &cameraMat, &cameraMat + 1);
	commonUBO["camera"] = cameraMat.mat; //AffineInverse(cameraMat.mat); //TODO
	commonUBO["projection"] = view.PerspMat();
	commonUBO.Bind();

	{
		auto bound = fbo.Bind(GL_FRAMEBUFFER);
		fbo.PreDraw(Vector4f{ 0, 0, 0, 0 },
			Eigen::Matrix<GLuint, 4, 1>{ Object::none.Id(), 0, 0, 0 });
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		r.Draw(alpha);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		for (auto& fn : customs) fn.second(alpha);
	}

	view.GlViewport();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	
	screenMat.use();
	screenQuad.Draw();
}

void RenderPasses::Highlight(Object o, Highlights type)
{
	screenMat.GetUBO()["selected"][type] = o.Id();
}

Object RenderPasses::Pick(Vector2f posPixel) const
{
	Eigen::Array2f posView = view.Pixel2View(posPixel).array();

	if ((posView < Eigen::Array2f::Zero()).any()
		|| (posView > Eigen::Array2f(1, 1)).any())
		return Object::invalid;

	return Object{ fbo.ReadPixel<std::uint32_t>(RenderPasses::PickerPass, posView) };
}