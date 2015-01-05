#include "stdafx.h"
#include "Wavefront.hpp"
#include "Window.hpp"
#include "Rendering/Render.hpp"
#include "Rendering/FBO.hpp"
#include "Geometry/Mesh.hpp"
#include "Geometry/AABB.hpp"
#include "Profiling.hpp"
#include "Mobile.hpp"

#include "GLFW/glfw3.h"

#include <iostream>
#include <chrono>

#include <unistd.h>

int main(void)
try
{
	Profile::CalibrateProfiling();

	//Components
	Window w;
    Render r;
    Mobile m;
    
    //load the object
	Object teapotObj, teapot2Obj;
	Wavefront teapot{"assets/capsule.obj"};

	//AABB teapotAabb(teapot.mesh);
	//Object aabbObj;
	//ShowAABB aabb(teapotAabb);

    ShaderProgram pickerShader {"assets/picker"};
    TypedTex<std::uint32_t> pickTex{TexDim{512, 512}};
    FBO<std::uint32_t> pickerFBO{pickTex};
    pickerFBO.AttachDepth(RenderBuffer{GL_DEPTH_COMPONENT, {512, 512}});
    pickerFBO.CheckStatus();

    Object pickerObj;
    ShaderProgram pickerHlShader("assets/picker_hl");
    UBO pickerMat = pickerHlShader.MakeUBO("Material", "pickerMat");
    pickerMat["selected"] = Object::none.Id();
    pickerMat.Sync();
    r.Create(pickerObj, {pickerHlShader, {}}, {{{pickerMat, {pickTex}}, {}}}, UnitBox, Matrix4f::Identity());

	auto locProxy = r.Create(teapotObj, {teapot.shaderProgram, pickerShader},
        {{{{}, {{"assets/capsule.png"}}}, {}}},
        teapot.vertexData, Matrix4f::Identity()*2);
	//auto locProxyAabb = r.Create(aabbObj, aabb.shaderProgram, {{}, {}},
	//	aabb.vertData, Matrix4f::Identity());
    
    r.Create(teapot2Obj, {teapot.shaderProgram, pickerShader},
        {{{{}, {{"assets/capsule.png"}}}, {}}},
        teapot.vertexData, //Matrix4f::Identity());
        Eigen::Affine3f(Eigen::Translation3f{2,0,0}).matrix()*3);

	auto moveProxy = m.Create(Transform(), {locProxy}); //{locProxy, locProxyAabb});
    
	using clock = std::chrono::system_clock;
	auto currentTime = clock::now();
	const auto dt = std::chrono::milliseconds(30);
	clock::duration accumulator(0);

	m.CameraLoc().pos = Vector3f(0.f, -3.f, 0.f);

	m.Tick();

    //our main loop
    while (!glfwWindowShouldClose(w.window))
    {
		auto newTime = clock::now();
		auto frameTime = newTime - currentTime;
		if (frameTime > std::chrono::milliseconds(250))
			frameTime = std::chrono::milliseconds(250);
		currentTime = newTime;

		accumulator += frameTime;

		while (accumulator >= dt)
		{
			w.GetInput();
			m.Tick();
			//physics step
			moveProxy->rot *= Quaternionf{ Eigen::AngleAxisf(0.04f, Vector3f::UnitY()) };

			if (glfwGetMouseButton(w.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
			{
				m.CameraLoc().rot *= Quaternionf{
					Eigen::AngleAxisf(-w.mouseDeltaPct().x()*3.f,
					Vector3f::UnitZ()) };
				m.CameraLoc().rot *= Quaternionf{
					Eigen::AngleAxisf(w.mouseDeltaPct().y()*3.f,
					m.CameraLoc().rot.conjugate() * Vector3f::UnitX()) };
			}

			accumulator -= dt;
		}

		using millifloat = std::chrono::duration<float, std::milli>;
		float alpha = std::chrono::duration_cast<millifloat>(accumulator).count()
			/ std::chrono::duration_cast<millifloat>(dt).count();
		m.Update(alpha);

        {
            auto bound = pickerFBO.Bind(GL_FRAMEBUFFER);
            pickerFBO.PreDraw({Object::none.Id(),0,0,0});
            r.DrawPass(PickerPass);
            pickerMat["selected"] = pickerFBO.ReadPixel(w.mousePosPct());
            pickerMat.Sync();
        }

		w.PreDraw();
		r.camera = w.PerspMat() * m.CameraMat();
		r.Draw();
		w.PostDraw();
    }
    
    return EXIT_SUCCESS;
}
catch (const char* mesg)
{
    std::cerr << mesg << "\n";
#ifndef __APPLE__
    std::cerr << "Press enter to exit...\n";
    getchar();
#endif
    return EXIT_FAILURE;
}
catch (std::exception &ex)
{
    std::cerr << ex.what() << "\n";
#ifndef __APPLE__
    std::cerr << "Press enter to exit...\n";
    getchar();
#endif
	return EXIT_FAILURE;
}
