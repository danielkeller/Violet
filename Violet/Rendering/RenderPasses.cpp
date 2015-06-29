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
{
	screenMat.Shader().TextureOrder({ "color", "picker" });
}

//can't just get it from window.view in case the editor is on
void RenderPasses::SetViewport(const Viewport& newView)
{
    Vector2i pixelSize = newView.PixelSize();
	if (view.PixelSize() != pixelSize)
    {
        auto mainTex = TypedTex<>{ pixelSize };
        auto pickerTex = TypedTex<std::uint32_t>{ pixelSize };

        fbo.AttachTexes({ mainTex, pickerTex });
        fbo.AttachDepth(RenderBuffer{ GL_DEPTH_COMPONENT, pixelSize });
        fbo.CheckStatus();

        screenMat.Textures() = { mainTex, pickerTex };
    }
    view = newView;
}

void RenderPasses::Camera(Object c)
{
	camera = c;
}

void RenderPasses::CreateCustom(Object obj, Custom draw)
{
	customs[obj] = draw;
}

void RenderPasses::Draw(const Viewport& view, float alpha)
{
	//just don't bother
	if ((view.PixelSize().array() == Eigen::Array2i::Zero()).any())
		return;

    SetViewport(view);

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

Object RenderPasses::Pick(Vector2f posSc) const
{
	Eigen::Array2f posTex = view.Sc2Tex(posSc).array();

	if ((posTex < Eigen::Array2f::Zero()).any()
		|| (posTex > Eigen::Array2f(1, 1)).any())
		return Object::invalid;

	return Object{ fbo.ReadPixel<std::uint32_t>(RenderPasses::PickerPass, posTex) };
}