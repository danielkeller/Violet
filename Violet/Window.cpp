#include "stdafx.h"
#include "Window.hpp"

#include "GLFW/glfw3.h"

#include <iostream>

void CheckGLError()
{
    GLenum err = glGetError();
    while (err != GL_NO_ERROR) {
        std::cerr << "Gl error [" << err << "] " << (err) << std::endl; //gluErrorString
        err = glGetError();
    }
}

static void error_callback(int error, const char* description)
{
    std::cerr << description << "\n";
    getchar();
}

Window::Window()
{
    //set up glfw
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) 
        throw "Could not initialize glfw";
    
    //only allow non-deprecated GL3.3 calls
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //create the window
    window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
    if (!window)
        throw "Could not create window";

    //set the window as current. note that this won't work right with multiple windows.
    glfwMakeContextCurrent(window);
    //set vsync
    //glfwSwapInterval(1);

    //load GL function pointers
    if(ogl_LoadFunctions() == ogl_LOAD_FAILED)
        throw "Error in glLoadGen";
}

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}