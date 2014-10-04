#include "stdafx.h"
#include "Wavefront.hpp"
#include "Window.hpp"
#include "Rendering/Render.hpp"
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
	Object teapot;
	VAO teapotVAO;
	ShaderProgram teapotShader;
	std::tie(teapotVAO, teapotShader) = LoadWavefront("assets/capsule.obj");

	teapotShader.TextureOrder({ "tex" });
	std::vector<Tex> texes;
		texes.emplace_back(Tex::create("assets/capsule.png"));

	auto locProxy = r.Create(teapot, teapotShader, std::make_tuple(UBO(), texes), teapotVAO, Matrix4f::Identity());

	auto moveProxy = m.Add(Transform(), locProxy);
    
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
			//moveProxy->rot *= Quaternionf{ Eigen::AngleAxisf(0.04f, Vector3f(0, 0, 1)) };

			if (glfwGetMouseButton(w.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
			{
				m.CameraLoc().rot *= Quaternionf{
					Eigen::AngleAxisf(-float(w.mouseDelta().x()),
					Vector3f::UnitZ()) };
				m.CameraLoc().rot *= Quaternionf{
					Eigen::AngleAxisf(float(w.mouseDelta().y()),
					m.CameraLoc().rot.conjugate() * Vector3f::UnitX()) };
			}

			accumulator -= dt;
		}

		using millifloat = std::chrono::duration<float, std::milli>;
		float alpha = std::chrono::duration_cast<millifloat>(accumulator).count()
			/ std::chrono::duration_cast<millifloat>(dt).count();
		m.Update(alpha);

		r.camera = w.PerspMat() * m.CameraMat();

		w.PreDraw();
		r.draw();
		w.PostDraw();

		printErr();
    }
    
    return EXIT_SUCCESS;
}
catch (const char* mesg)
{
    std::cerr << mesg << "\nPress enter to exit...\n";
    getchar();
    return EXIT_FAILURE;
}
catch (std::exception ex)
{
	std::cerr << ex.what() << "\nPress enter to exit...\n";
	getchar();
	return EXIT_FAILURE;
}