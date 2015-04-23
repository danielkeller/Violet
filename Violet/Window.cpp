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

Vector2f Events::MouseDeltaScr() const
{
	return MouseDeltaView() * 2.f;
}

Vector2f Events::MousePosScr() const
{
	return mouseCur.array() / dimVec.cast<float>().array() * 2.f - 1.f;
}

Vector2f Events::MouseDeltaView() const
{
	return (mouseCur - mouseOld).array() / dimVec.cast<float>().array();
}

Vector2f Events::MousePosView() const
{
	return mouseCur.array() / dimVec.cast<float>().array();
}

Vector2f Events::MouseDeltaPxl() const
{
	return mouseCur - mouseOld;
}

Vector2f Events::MousePosPxl() const
{
	return Vector2f(mouseCur.x(), float(dimVec.y()) - mouseCur.y());
}

Vector2f Events::ScrollDelta() const
{
	return scrollAmt;
}

bool Events::MouseButton(int num) const
{
	return mouseButtonsCur[num];
}

bool Events::MouseClick(int num) const
{
	return mouseButtonsCur[num] && !mouseButtonsOld[num];
}

bool Events::MouseRelease(int num) const
{
	return !mouseButtonsCur[num] && mouseButtonsOld[num];
}

void Events::PopMouse()
{
	//mouseCur << -1, -1;
	mouseOld << mouseCur;
	for (auto& b : mouseButtonsCur) b = false;
	for (auto& b : mouseButtonsOld) b = false;
}

void Events::PopScroll()
{
	scrollAmt << 0, 0;
}

void Events::Step()
{
	keyEvents.clear();
	charEvents.clear();
	PopScroll();

	mouseButtonsOld = mouseButtonsCur;
	mouseOld = mouseCur;
}

Matrix4f Events::PerspMat() const
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

Window* getWindow(GLFWwindow* window)
{
	return static_cast<Window*>(glfwGetWindowUserPointer(window));
}

static void error_callback(int error, const char* description)
{
    std::cerr << description << "\n";
    getchar();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	getWindow(window)->newEvents.keyEvents.emplace_back(
		KeyEvent{ { key, scancode, mods }, action });

	if (action != GLFW_PRESS)
		return;
	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, true);
		break;
	default:
		break;
	}
}

void character_callback(GLFWwindow* window, unsigned int codepoint)
{
	getWindow(window)->newEvents.charEvents.push_back(codepoint);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	getWindow(window)->newEvents.scrollAmt += Vector2f(xoffset, yoffset);
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
{
	newEvents.dimVec << 1024, 768;

	accessor<Eigen::Vector2i> dimAcc = {
		[this]() { return newEvents.dimVec; },
		[this](Eigen::Vector2i d) {newEvents.dimVec = d; }
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
	//glfwWindowHint(GLFW_DECORATED, false);

    //create the window
	window = glfwCreateWindow(newEvents.dimVec.x(), newEvents.dimVec.y(),
		"Simple example", NULL, NULL);
    if (!window)
		throw "Could not create window";

	GetInput();

	glfwSetWindowUserPointer(window, this);

	//add our input callbacks
	glfwSetKeyCallback(window, key_callback);
	glfwSetCharCallback(window, character_callback);
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

	GetInput();
}

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

Events Window::GetInput()
{
	//gather any events that happened that we didn't catch yet.
	glfwPollEvents();

	//get the mouse position too
	double x, y;
	glfwGetCursorPos(window, &x, &y);
	//opengl and glfw use opposite viewport coordinates
	newEvents.mouseCur << float(x), float(newEvents.dimVec.y() - y);

	for (int b = 0; b < newEvents.mouseButtonsCur.size(); ++b)
		newEvents.mouseButtonsCur[b] = glfwGetMouseButton(window, b) == GLFW_PRESS;

	//make events that happened visible
	Events ret = newEvents;
	//clear out the events buffer for the coming frame
	newEvents.Step();

	return ret;
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
#ifndef GL_DEBUG
    //Check errors normally
    CheckGLError();
#endif
}

Matrix4f Window::PerspMat() const
{
	return newEvents.PerspMat();
}

Matrix4f PixelMat(Vector2i dim)
{
	Matrix4f ret;
	ret <<
		2.f / float(dim.x()), 0, 0, -1,
		0, -2.f / float(dim.y()), 0, 1,
		0, 0, 1, 0,
		0, 0, 0, 1;
	return ret;
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(window) != 0;
}