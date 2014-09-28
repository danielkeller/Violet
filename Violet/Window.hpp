#ifndef WINDOW_H
#define WINDOW_H

struct GLFWwindow;

class Window
{
public:
    //Window initialization and cleanup
    Window();
    ~Window();

    GLFWwindow* window;
};

//print out a message if there are GL errors
void CheckGLError();

#endif