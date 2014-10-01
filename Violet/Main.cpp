#include "stdafx.h"
#include "Wavefront.hpp"
#include "Window.hpp"
#include "GLMath.h"
#include "Rendering/Render.hpp"
#include "Profiling.hpp"
#include "Mobile.hpp"

#include "GLFW/glfw3.h"

#include <iostream>
#include <chrono>

#define _USE_MATH_DEFINES
#include <cmath>

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS)
        return;
    switch (key)
    {
    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GL_TRUE);
        break;
    default:
        break;
    }
}

int main(void)
try
{
	Profile::CalibrateProfiling();

    //create the basic window
    Window w;
    //add our keyboard input callback
    glfwSetKeyCallback(w.window, key_callback);

	Render r;

    //load the Render
	Object teapot;
	VAO teapotVAO;
	ShaderProgram teapotShader;
	std::tie(teapotVAO, teapotShader) = LoadWavefront("assets/capsule.obj");

	teapotShader.TextureOrder({ "tex" });
	std::vector<Tex> texes;
		texes.emplace_back(Tex::create("assets/capsule.png"));

	auto locProxy = r.Create(teapot, teapotShader, std::make_tuple(UBO(), texes), teapotVAO, Matrix4f::Identity());

	Mobile m;
	auto moveProxy = m.Add(Transform(), locProxy);

    //clear to black
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    
	using clock = std::chrono::system_clock;
	auto currentTime = clock::now();
	const auto dt = std::chrono::milliseconds(30);
	clock::duration accumulator(0);

    double oldX, oldY;
    glfwGetCursorPos(w.window, &oldX, &oldY);

    Matrix4f rotMat = Matrix4f::Identity();

    //our main loop
    while (!glfwWindowShouldClose(w.window))
    {
        //set the GL draw surface to the same size as the window
        int width, height;
        glfwGetFramebufferSize(w.window, &width, &height);
		glViewport(0, 0, (GLsizei)width, (GLsizei)height);

		auto newTime = clock::now();
		auto frameTime = newTime - currentTime;
		if (frameTime > std::chrono::milliseconds(250))
			frameTime = std::chrono::milliseconds(250);
		currentTime = newTime;

		accumulator += frameTime;

		while (accumulator >= dt)
		{
			m.Tick();
			//physics step
			moveProxy->rot *= Quaternionf{ Eigen::AngleAxisf(0.04f, Vector3f(0, 0, 1)) };

			double curX, curY;
			glfwGetCursorPos(w.window, &curX, &curY);
			if (glfwGetMouseButton(w.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
				rotMat *= Eigen::Affine3f(Eigen::AngleAxisf(float(curX - oldX) / float(width), Vector3f::UnitY())).matrix();
			oldX = curX, oldY = curY;

			accumulator -= dt;
		}

		using millifloat = std::chrono::duration<float, std::milli>;
		float alpha = std::chrono::duration_cast<millifloat>(accumulator).count()
			/ std::chrono::duration_cast<millifloat>(dt).count();
		m.Update(alpha);

        //set the camera matrix
        Matrix4f cameraMat = lookAt(
            Vector3f(0.f, 0.f, 2.f),
            Vector3f::Zero(),
            Vector3f(0.f, 1.f, 0.f));

        //set the perspective matrix
        Matrix4f perspMat = perspective((float)M_PI/2.f, (float)width / height, .01f, 100.f);

		r.camera = perspMat * cameraMat * rotMat;
		r.draw();

        //swap draw buffer and visible buffer
        glfwSwapBuffers(w.window);
        glfwPollEvents();

		printErr();
    }

	//std::cin.get();
    
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