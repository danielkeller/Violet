#include "stdafx.h"
#include "Window.hpp"
#include "GLMath.h"

#include "Profiling.hpp"

#include "GLFW/glfw3.h"

#include <iostream>

#ifndef _WIN32
#include "posixStackTrace.hpp"
#endif

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

#ifdef GL_DEBUG
void APIENTRY glDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const char* message, const void* userParam)
{
	std::cerr << "GL Error caught '" << message << "'\n";
#ifndef _WIN32
	printStackTrace();
#endif
}
#endif

Window* getWindow(GLFWwindow* window)
{
	return static_cast<Window*>(glfwGetWindowUserPointer(window));
}

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
		glfwSetWindowShouldClose(window, true);
		break;
	default:
		break;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	getWindow(window)->scrollAmt += Vector2f(xoffset, yoffset);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	if (width != 0 && height != 0)
	{
		glViewport(0, 0, width, height);
		getWindow(window)->dim.set(Eigen::Vector2i(width, height));
	}
}

Window::Window()
	: dimVec(640, 480)
{
	accessor<Eigen::Vector2i> dimAcc = {
		[this]() { return dimVec; },
		[this](Eigen::Vector2i d) {dimVec = d; }
	};
	dim = dimAcc;

    //set up glfw
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) 
        throw "Could not initialize glfw";
    
    //only allow non-deprecated GL3.3 calls
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
#endif
#ifdef GL_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

    //create the window
	window = glfwCreateWindow(dimVec.x(), dimVec.y(), "Simple example", NULL, NULL);
    if (!window)
		throw "Could not create window";

	GetInput();

	glfwSetWindowUserPointer(window, this);

	//add our input callbacks
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    //set the window as current. note that this won't work right with multiple windows.
    glfwMakeContextCurrent(window);
    //set vsync
    glfwSwapInterval(1);

#ifdef GL_DEBUG
    //enable for all errors
	glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	//glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW_ARB,
	//	0, nullptr, GL_FALSE);
    //get stacktrace in correct thread & function
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallbackARB(glDebugProc, nullptr);
#endif

	//Draw the correct sides of things
	glCullFace(GL_FRONT_AND_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	//clear to black
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	ClearInput();
}

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::GetInput()
{
	mouseOld = mouseCur;
    double x, y;
	glfwGetCursorPos(window, &x, &y);
	//opengl and glfw use opposite viewport coordinates
	mouseCur << float(x), float(dimVec.y() - y);
}

void Window::ClearInput()
{
	scrollAmt = Vector2f::Zero();
}

Vector2f Window::MouseDeltaScr() const
{
    return MouseDeltaView() * 2.f;
}

Vector2f Window::MousePosScr() const
{
	return mouseCur.array() / dimVec.cast<float>().array() * 2.f - 1.f;
}

Vector2f Window::MouseDeltaView() const
{
	return (mouseCur - mouseOld).array() / dimVec.cast<float>().array();
}

Vector2f Window::MousePosView() const
{
	return mouseCur.array() / dimVec.cast<float>().array();
}

Vector2f Window::MouseDeltaPxl() const
{
    return mouseCur - mouseOld;
}

Vector2f Window::MousePosPxl() const
{
    return mouseCur;
}

Vector2f Window::ScrollDelta() const
{
	return scrollAmt;
}

void Window::PreDraw()
{
	//clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::PostDraw()
{
	auto p = Profile::Profile("waiting for vsync");
	//swap draw buffer and visible buffer
	glfwSwapBuffers(window);
	glfwPollEvents();
#ifndef GL_DEBUG
    //Check errors normally
    CheckGLError();
#endif
}

Matrix4f Window::PerspMat() const
{
	Matrix4f z_upToY_up;
	z_upToY_up <<
		1, 0, 0, 0,
		0, 0, 1, 0,
		0, 1, 0, 0,
		0, 0, 0, 1;
	return perspective((float)M_PI / 2.f, (float)dimVec.x() / dimVec.y(),
		.01f, 100.f) * z_upToY_up;
}

bool Window::LeftMouse() const
{
    return glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
}

bool Window::RightMouse() const
{
    return glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(window) != 0;
}