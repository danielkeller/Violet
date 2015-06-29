#include "stdafx.h"
#include "Assets.hpp"

#include "Rendering/Render.hpp"
#include "Window.hpp"

using namespace Asset_detail;

Tex ObjAssets::Thumb(const std::string& path)
{
	static const TexDim dim{ THM_SIZE, THM_SIZE };
    Viewport view = UI::FrameEvents().view.SubView({ Vector2i::Zero(), dim });

	static ShaderProgram shader{ "assets/shaded" };
	static UBO mat{ shader, "Material" };
	static UBO cam{ shader, "Common" };
	STATIC
	{
		mat["light"] = Vector3f{ -1, 1, 1 }.normalized();
		cam["camera"] = Matrix4f::Identity();
    }
    
    cam["projection"] = view.PerspMat();

	static InstData object{ Object::invalid,
		Eigen::Affine3f{ Eigen::Translation3f{ 0, -3, 0 } }.matrix() };
	static BufferObject<InstData, GL_ARRAY_BUFFER, GL_STATIC_DRAW> instances(1);
	STATIC
		instances.Assign(0, object);

	//todo: more efficient to reuse the VAO
	VAO vao{ shader, { path } };
	vao.BindInstanceData(shader, instances);

	static FBO fbo;

    fbo.AttachDepth(RenderBuffer{ GL_DEPTH_COMPONENT, view.PixelSize() });

	TypedTex<> ret{ view.PixelSize() };
	fbo.AttachTexes({ ret });
	fbo.CheckStatus();

	auto bound = fbo.Bind(GL_FRAMEBUFFER);
	fbo.PreDraw(Vector4f{ 0, 0, 0, 0 });
	shader.use();
	cam.Bind();
	mat.Bind();

	//Draw the correct sides of things
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	vao.Draw();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	return ret;
}

Tex MaterialAssets::Thumb(const Material& mat)
{
    static const TexDim dim{ THM_SIZE, THM_SIZE };
    Viewport view = UI::FrameEvents().view.SubView({ Vector2i::Zero(), dim });

	static UBO cam{ mat.Shader(), "Common" };
	STATIC
	{
		cam["camera"] = Matrix4f::Identity();
		cam["projection"] = Matrix4f::Identity();
	}

	//look down the end of it
	static InstData object{ Object::invalid, Eigen::Affine3f{
		Eigen::Translation3f(0, 0, -2) * Eigen::Scaling(2.f) }.matrix() };
	static BufferObject<InstData, GL_ARRAY_BUFFER, GL_STATIC_DRAW> instances(1);
	STATIC
		instances.Assign(0, object);

	VAO vao{ mat.Shader(), { "assets/capsule.obj" } };
	vao.BindInstanceData(mat.Shader(), instances);

	static FBO fbo;
    
    fbo.AttachDepth(RenderBuffer{ GL_DEPTH_COMPONENT, view.PixelSize() });

	TypedTex<> ret{ view.PixelSize() };
	fbo.AttachTexes({ ret });
	fbo.CheckStatus();

	auto bound = fbo.Bind(GL_FRAMEBUFFER);
	fbo.PreDraw(Vector4f{ 0, 0, 0, 0 });
	mat.use();
	cam.Bind();
	vao.Draw();

	return ret;
}