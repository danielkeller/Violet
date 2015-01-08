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
    //values are in Opengl screen coordinates
	Vector2f MouseDeltaScr() const;
    Vector2f MousePosScr() const;
    //values are in Opengl viewport coordinates
    Vector2f MouseDeltaView() const;
    Vector2f MousePosView() const;
    //values are in Opengl viewport coordinates in pixel units
    Vector2f MouseDeltaPxl() const;
    Vector2f MousePosPxl() const;
    
    Vector2i Dim();

    bool LeftMouse() const;
    bool RightMouse() const;
    
    bool ShouldClose() const;

private:
    GLFWwindow* window;
    Vector2i dim;
    
	Vector2f mouseOld, mouseCur;
};

//print out a message if there are GL errors
void CheckGLError();

#endif
