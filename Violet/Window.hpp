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
	Eigen::Vector2i dim;

	Matrix4f PerspMat();
    //values are in Opengl viewport coordinates
	Vector2f mouseDeltaPct();
	Vector2f mousePosPct();


private:
	Vector2f mouseOld, mouseCur;
};

//print out a message if there are GL errors
void CheckGLError();

#endif
