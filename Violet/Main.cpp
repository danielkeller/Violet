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
    
    auto then = std::chrono::system_clock::now();

    double oldX, oldY;
    glfwGetCursorPos(w.window, &oldX, &oldY);

    Matrix4f rotMat = Matrix4f::Identity();

    //our main loop
    while (!glfwWindowShouldClose(w.window))
    {
        //set the GL draw surface to the same size as the window
        int width, height;
        glfwGetFramebufferSize(w.window, &width, &height);
        glViewport(0, 0, (GLsizei) width, (GLsizei) height);

        float ang = (float)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - then).count() / 1000.f;
		then = std::chrono::system_clock::now();

		moveProxy->rot *= Quaternionf{ Eigen::AngleAxisf(ang, Vector3f(0, 0, 1)) };
		m.Update(0);
		m.Tick();

        double curX, curY;
        glfwGetCursorPos(w.window, &curX, &curY);

        if (glfwGetMouseButton(w.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            rotMat *= Eigen::Affine3f(Eigen::AngleAxisf(float(curX - oldX)/float(width), Vector3f::UnitY())).matrix();

        oldX = curX, oldY = curY;

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