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
		Vector4f worldVec = screenAxes.householderQr().solve(screenVec);
		return worldVec.block<3, 1>(0, 0) / worldVec[3];
	}
    
	Vector2f ScrollDelta() const;

    Vector2i Dim();

    bool LeftMouse() const;
    bool RightMouse() const;
    
    bool ShouldClose() const;

private:
    GLFWwindow* window;
    Vector2i dim;
    
	Vector2f mouseOld, mouseCur, scrollAmt;

	friend void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};

//print out a message if there are GL errors
void CheckGLError();

#endif
