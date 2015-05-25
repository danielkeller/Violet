#include "stdafx.h"
#include "Window.hpp"

#include "Utils/Profiling.hpp"

#include <iostream>
#include <iomanip>

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
		if (gluErrorString(err))
			std::cerr << "GL Error '" << gluErrorString(err) << "'\n";
		else
			std::cerr << "Other GL error: 0x" << std::hex << err << '\n';
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
	return view.Pixel2Scr(mouseCur) - view.Pixel2Scr(mouseOld);
}

Vector2f Events::MousePosScr() const
{
	return view.Pixel2Scr(mouseCur);
}

Vector2f Events::MouseDeltaView() const
{
	return view.Pixel2View(mouseCur) - view.Pixel2View(mouseOld);
}

Vector2f Events::MousePosView() const
{
	return view.Pixel2View(mouseCur);
}

Vector2f Events::MouseDeltaPxl() const
{
	return mouseCur - mouseOld;
}

Vector2f Events::MousePosPxl() const
{
	return mouseCur;
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
	mouseCur << -1, -1;
	mouseOld << mouseCur;
	for (auto& b : mouseButtonsCur) b = false;
	for (auto& b : mouseButtonsOld) b = false;
}

void Events::PopScroll()
{
	scrollAmt << 0, 0;
}

bool Events::HasKeyEvent(KeyEvent key)
{
	if (key.action == RELEASE_OR_REPEAT)
		return HasKeyEvent({ key.key, GLFW_REPEAT }) || HasKeyEvent({ key.key, GLFW_RELEASE });

	return std::find(keyEvents.begin(), keyEvents.end(), key)
		!= keyEvents.end();
}

bool Events::PopKeyEvent(KeyEvent key)
{
	if (key.action == RELEASE_OR_REPEAT) //only pop one of them
		return PopKeyEvent({ key.key, GLFW_REPEAT }) || PopKeyEvent({ key.key, GLFW_RELEASE });

	auto ev = std::find(keyEvents.begin(), keyEvents.end(), key);
	if (ev == keyEvents.end())
		return false;
	keyEvents.erase(ev);
	return true;
}

bool Events::PopKey(Key key)
{
	return PopKeyEvent({ key, RELEASE_OR_REPEAT });
}

void Events::Step()
{
	keyEvents.clear();
	charEvents.clear();
	PopScroll();

	mouseButtonsOld = mouseButtonsCur;
	mouseOld = mouseCur;
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

void cursor_enter_callback(GLFWwindow* window, int entered)
{
	getWindow(window)->newEvents.mouseCur.setConstant(-1.f);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	getWindow(window)->newEvents.mouseCur <<
		float(xpos), float(getWindow(window)->newEvents.dimVec.y() - ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	getWindow(window)->newEvents.scrollAmt += Vector2f(xoffset, yoffset);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	if (width != 0 && height != 0)
		getWindow(window)->dim.set(Eigen::Vector2i(width, height));
}

void character_callback(GLFWwindow* window, unsigned int codepoint)
{
	getWindow(window)->newEvents.charEvents.push_back(codepoint);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	auto& events = getWindow(window)->newEvents;
	events.keyEvents.emplace_back(
		KeyEvent{ { key, mods }, action });

	if (action != GLFW_PRESS)
		return;

	if (mods & GLFW_MOD_ALT && key == GLFW_KEY_F4)
		glfwSetWindowShouldClose(window, true);
}

#ifdef MAGIC
#include "Windows.hpp"
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "GLFW/glfw3native.h"
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")
#endif

Window::Window()
{
	newEvents.dimVec << 1024, 768;
	newEvents.view.screenBox = { Vector2i::Zero(), newEvents.dimVec };

	accessor<Eigen::Vector2i> dimAcc = {
		[this]() { return newEvents.dimVec; },
		[this](Eigen::Vector2i d) {newEvents.dimVec = d; }
	};
	dim = dimAcc;

    //set up glfw
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) 
        throw std::runtime_error("Could not initialize glfw");
    
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
		throw std::runtime_error("Could not create window");

#ifdef MAGIC
	HWND hWnd = glfwGetWin32Window(window);
	DWORD style = ::GetWindowLong(hWnd, GWL_STYLE);
	style &= ~WS_OVERLAPPEDWINDOW;
	style |= WS_POPUP;
	::SetWindowLong(hWnd, GWL_STYLE, style);
	DWM_BLURBEHIND bb = { 0 };
	bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
	bb.fEnable = true;
	bb.hRgnBlur = CreateRectRgn(0, 0, 1, 1);
	DwmEnableBlurBehindWindow(hWnd, &bb);
#endif

	glfwSetWindowUserPointer(window, this);

	//add our input callbacks
	glfwSetCursorEnterCallback(window, cursor_enter_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
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

	//set and forget stuff
	//glClearColor(1.f, 0.f, 1.f, 1.f);
	glCullFace(GL_FRONT);
	glDepthFunc(GL_LESS);

	GetInput();
}

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::SetView(Viewport view)
{
	newEvents.view = view;
}

void Window::SetTime(Time::clock::duration simTime)
{
	newEvents.simTime = simTime;
}

Events Window::GetInput()
{
	//gather any events that happened that we didn't catch yet.
	glfwPollEvents();

	for (size_t b = 0; b < newEvents.mouseButtonsCur.size(); ++b)
		newEvents.mouseButtonsCur[b] = glfwGetMouseButton(window, int(b)) == GLFW_PRESS;
	
	newEvents.scrollPopped = newEvents.mousePopped = false;

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

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(window) != 0;
}