#include "stdafx.h"
#include "Assets.hpp"

#include "Rendering/Render.hpp"

Tex ObjAssets::Thumb(const std::string& path)
{
	static const TexDim dim{ Assets::THM_SIZE, Assets::THM_SIZE };
	static Viewport view({ Vector2i::Zero(), dim });

	static ShaderProgram shader{ "assets/shaded" };
	static UBO mat = shader.MakeUBO("Material", "ShadedLight");
	static UBO cam = shader.MakeUBO("Common", "ThumbCamera");
	STATIC
	{
		mat["light"] = Vector3f{ -1, 1, 1 }.normalized();
		cam["camera"] = view.PerspMat();
	}

	static InstData object{ Object::invalid,
		Eigen::Affine3f{ Eigen::Translation3f{ 0, -3, 0 } }.matrix() };
	static BufferObject<InstData, GL_ARRAY_BUFFER, GL_STATIC_DRAW> instances(1);
	STATIC
		instances.Assign(0, object);

	//todo: more efficient to reuse the VAO
	VAO vao{ shader, { path } };
	vao.BindInstanceData(shader, instances);

	static FBO fbo;

	STATIC
		fbo.AttachDepth(RenderBuffer{ GL_DEPTH_COMPONENT, dim });

	TypedTex<> ret{ dim };
	fbo.AttachTexes({ ret });
	fbo.CheckStatus();

	auto bound = fbo.Bind(GL_FRAMEBUFFER);
	fbo.PreDraw(Vector4f{ 0, 0, 0, 0 });
	shader.use();
	cam.Bind();
	mat.Bind();

	//Draw the correct sides of things
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	vao.Draw();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	return ret;
}