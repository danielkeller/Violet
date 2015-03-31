#ifndef WINDOW_H
#define WINDOW_H

struct GLFWwindow;

#include "magic_ptr.hpp"

class Window
{
public:
    //Window initialization and cleanup
    Window();
    ~Window();

	void GetInput();
	void ClearInput();
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

	template<class Derived>
	Vector3f ApparentMousePos(const Eigen::MatrixBase<Derived>& modelview)
	{
		auto screenAxes = PerspMat() * modelview;
		Vector4f screenVec;
		screenVec << MousePosScr()*screenAxes(3, 3), screenAxes(2, 3), screenAxes(3, 3);
		Vector4f worldVec(screenAxes.householderQr().solve(screenVec));
		return worldVec.block<3, 1>(0, 0) / worldVec[3];
	}
    
	Vector2f ScrollDelta() const;

    magic_ptr<Vector2i> dim;

	bool LeftMouse() const;
	bool LeftMouseClick() const;
	bool LeftMouseRelease() const;
	bool RightMouse() const;
	bool RightMouseClick() const;
	bool RightMouseRelease() const;
    
    bool ShouldClose() const;

private:
    GLFWwindow* window;
    Vector2i dimVec;
    
	bool leftMouseOld, leftMouseCur, rightMouseOld, rightMouseCur;
	Vector2f mouseOld, mouseCur, scrollAmt;

	friend void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	friend void framebuffer_size_callback(GLFWwindow* window, int width, int height);
};

//print out a message if there are GL errors
void CheckGLError();

//ortho matrix for pixel drawing
Matrix4f PixelMat(Vector2i dim);

#endif
