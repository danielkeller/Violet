#ifndef WINDOW_H
#define WINDOW_H

struct GLFWwindow;

#include "magic_ptr.hpp"
#include <deque>
#include <array>

//Copied from GLFW
#define MOUSE_BUTTON_1         0
#define MOUSE_BUTTON_2         1
#define MOUSE_BUTTON_3         2
#define MOUSE_BUTTON_4         3
#define MOUSE_BUTTON_5         4
#define MOUSE_BUTTON_6         5
#define MOUSE_BUTTON_7         6
#define MOUSE_BUTTON_8         7
#define MOUSE_BUTTON_LAST      MOUSE_BUTTON_8
#define MOUSE_BUTTON_LEFT      MOUSE_BUTTON_1
#define MOUSE_BUTTON_RIGHT     MOUSE_BUTTON_2
#define MOUSE_BUTTON_MIDDLE    MOUSE_BUTTON_3

class Window;

struct Key
{
	int key;
	int scancode;
	int mods;
};

struct KeyEvent
{
	Key key;
	int action;
};

class Events
{
public:
	bool MouseButton(int num) const;
	bool MouseClick(int num) const;
	bool MouseRelease(int num) const;

	//values are in Opengl screen coordinates
	Vector2f MouseDeltaScr() const;
	Vector2f MousePosScr() const;
	//values are in Opengl viewport coordinates
	Vector2f MouseDeltaView() const;
	Vector2f MousePosView() const;
	//values are in Opengl viewport coordinates in pixel units
	Vector2f MouseDeltaPxl() const;
	Vector2f MousePosPxl() const;

	Vector2f ScrollDelta() const;

	void PopMouse();
	void PopScroll();

	template<class Derived>
	Vector3f ApparentMousePos(const Eigen::MatrixBase<Derived>& modelview) const
	{
		auto screenAxes = PerspMat() * modelview;
		Vector4f screenVec;
		screenVec << MousePosScr()*screenAxes(3, 3), screenAxes(2, 3), screenAxes(3, 3);
		Vector4f worldVec(screenAxes.householderQr().solve(screenVec));
		return worldVec.block<3, 1>(0, 0) / worldVec[3];
	}

	bool PopKeyEvent(KeyEvent);
	unsigned int PopCharEvent();

	Matrix4f PerspMat() const;

public:
	Vector2i dimVec;

	using MouseBottons = std::array<bool, 5>;

	MouseBottons mouseButtonsOld, mouseButtonsCur;
	Vector2f mouseOld, mouseCur, scrollAmt;
	bool mousePopped, scrollPopped;
	
	//std::vector<Key> heldKeys;
	std::deque<KeyEvent> keyEvents;
	std::deque<unsigned int> charEvents;

	void Step();

	friend class Window;
};

class Window
{
public:
    //Window initialization and cleanup
    Window();
    ~Window();

	Events GetInput();
	void PreDraw();
	void PostDraw();

	Matrix4f PerspMat() const;

    magic_ptr<Vector2i> dim;
    
    bool ShouldClose() const;

private:
    GLFWwindow* window;

	Events newEvents;

	friend void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	friend void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	friend void character_callback(GLFWwindow* window, unsigned int codepoint);
	friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

//print out a message if there are GL errors
void CheckGLError();

//ortho matrix for pixel drawing
Matrix4f PixelMat(Vector2i dim);

#endif
