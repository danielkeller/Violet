#ifndef WINDOW_H
#define WINDOW_H

struct GLFWwindow;

class Window
{
public:
    //Window initialization and cleanup
    Window();
    ~Window();

	void GetInput();
	void PreDraw();
	void PostDraw();

	GLFWwindow* window;
	int width, height;

	Matrix4f PerspMat();
	Eigen::Vector2d mouseDelta();


private:
	Eigen::Vector2d mouseOld, mouseCur;
};

//print out a message if there are GL errors
void CheckGLError();

#endif