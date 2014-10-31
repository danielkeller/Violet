#include "stdafx.h"
#include "Window.hpp"
#include "GLMath.h"

#include "GLFW/glfw3.h"

#include <iostream>

//Instead of including windows' GLU, just define the one useful function
//and link against it
extern "C" const GLubyte* APIENTRY gluErrorString(GLenum errCode);

void CheckGLError()
{
    GLenum err = glGetError();
    while (err != GL_NO_ERROR) {
		std::cerr << "GL Error '" << gluErrorString(err) << "'\n";
        err = glGetError();
    }
}

/*
void APIENTRY glDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const char* message, const void* userParam)
{
    std::cerr << "GL Error caught '" << message << "'\n";
}
*/

static void error_callback(int error, const char* description)
{
    std::cerr << description << "\n";
    getchar();
}

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

Window::Window()
{
    //set up glfw
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) 
        throw "Could not initialize glfw";
    
    //only allow non-deprecated GL3.3 calls
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //create the window
    window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
    if (!window)
		throw "Could not create window";

	glfwGetCursorPos(window, &mouseOld[0], &mouseOld[1]);

	//add our keyboard input callback
	glfwSetKeyCallback(window, key_callback);

    //set the window as current. note that this won't work right with multiple windows.
    glfwMakeContextCurrent(window);
    //set vsync
    //glfwSwapInterval(1);

    //load GL function pointers
    int numFailed = ogl_LoadFunctions() - ogl_LOAD_SUCCEEDED;
    std::cerr << numFailed << " GL functions failed to load\n";
    //if(ogl_LoadFunctions() != ogl_LOAD_FAILED)
    //    throw "Error in glLoadGen";

    CheckGLError();
    /*
    std::cerr << "setting debug callback\n";
    glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallbackARB(glDebugProc, nullptr);
    */

	//Draw the correct sides of things
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	//clear to black
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::GetInput()
{
	mouseOld = mouseCur;
	glfwGetCursorPos(window, &mouseCur[0], &mouseCur[1]);
}

Eigen::Vector2d Window::mouseDelta()
{
	return (mouseCur - mouseOld).array() / Eigen::Vector2d(width, height).array();
}

void Window::PreDraw()
{
	//set the GL draw surface to the same size as the window
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

void Window::PostDraw()
{
	//swap draw buffer and visible buffer
	glfwSwapBuffers(window);
	glfwPollEvents();
    CheckGLError();
}

Matrix4f Window::PerspMat()
{
	Matrix4f z_upToY_up;
	z_upToY_up <<
		1, 0, 0, 0,
		0, 0,-1, 0,
		0, 1, 0, 0,
		0, 0, 0, 1;
	return perspective((float)M_PI / 2.f, (float)width / height, .01f, 100.f) * z_upToY_up;
}
