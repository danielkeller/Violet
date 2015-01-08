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

	Matrix4f PerspMat() const;
    //values are in Opengl viewport coordinates
	Vector2f mouseDeltaPct() const;
	Vector2f mousePosPct() const;

    bool LeftMouse() const;
    bool RightMouse() const;
    
    bool ShouldClose() const;

private:
    GLFWwindow* window;
    Eigen::Vector2i dim;
    
	Vector2f mouseOld, mouseCur;
};

//print out a message if there are GL errors
void CheckGLError();

#endif
