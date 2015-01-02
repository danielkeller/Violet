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

int main(void)
try
{
	Profile::CalibrateProfiling();

	//Components
	Window w;
	Render r;
	Mobile m;

    //load the Render
	Object teapotObj, teapot2Obj;
	Wavefront teapot{"assets/capsule.obj"};

	//AABB teapotAabb(teapot.mesh);
	//Object aabbObj;
	//ShowAABB aabb(teapotAabb);

    ShaderProgram pickerShader {"assets/picker"};
    TypedTex<RGBA8Px> pickTex{TexDim{512, 512}};
    FBO<RGBA8Px> pickerFBO{pickTex};
    pickerFBO.AttachDepth(RenderBuffer{GL_DEPTH_COMPONENT, {512, 512}});
    pickerFBO.CheckStatus();
    Object pickerObj;
    ShaderProgram pickerDebugShader("assets/picker_debug");
    r.Create(pickerObj, {pickerDebugShader, {}}, {{{{}, {pickTex}}, {}}}, UnitBox, Matrix4f::Identity());

    pickerShader = teapot.shaderProgram;

	auto locProxy = r.Create(teapotObj, {teapot.shaderProgram, pickerShader},
        //{{{{}, {{"assets/capsule.png"}}}, {}}},
        {{{{}, {{"assets/capsule.png"}}}, {{}, {{"assets/capsule.png"}}}}},
        teapot.vertexData, Matrix4f::Identity());
	//auto locProxyAabb = r.Create(aabbObj, aabb.shaderProgram, {{}, {}},
	//	aabb.vertData, Matrix4f::Identity());

	r.Create(teapot2Obj, {teapot.shaderProgram, pickerShader},
        {{{{}, {{"assets/capsule.png"}}}, {{}, {{"assets/capsule.png"}}}}},
        teapot.vertexData, //Matrix4f::Identity());
        Eigen::Affine3f(Eigen::Translation3f{2,0,0}).matrix());

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

            //std::cout << pickerFBO.ReadPixel(w.mousePosPct()) << '\n';

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
            auto bound = pickerFBO.Bind(GL_DRAW_FRAMEBUFFER);
            pickerFBO.PreDraw();
            r.DrawPass(PickerPass);
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
    std::cerr << mesg << "\nPress enter to exit...\n";
    getchar();
    return EXIT_FAILURE;
}
catch (std::exception &ex)
{
	std::cerr << ex.what() << "\nPress enter to exit...\n";
	getchar();
	return EXIT_FAILURE;
}
