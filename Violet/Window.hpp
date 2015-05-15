#ifndef WINDOW_H
#define WINDOW_H

#include "Viewport.hpp"
#include "Core/Time.hpp"

#include "magic_ptr.hpp"
#include <deque>
#include <array>

//for event stuff
#include "GLFW/glfw3.h"

class Window;

struct Key
{
	int key;
	int mods;
	POD_EQUALITY(Key);
};

//key action for Events::PopKeyEvent
#define RELEASE_OR_REPEAT 3

struct KeyEvent
{
	Key key;
	int action;
	POD_EQUALITY(KeyEvent);
};

struct Events
{
	bool MouseButton(int num) const;
	bool MouseClick(int num) const;
	bool MouseRelease(int num) const;

	//values are in Opengl screen coordinates
	Vector2f MouseDeltaScr() const;
	Vector2f MousePosScr() const;
	//values are in Opengl viewport coordinates
	Vector2f MouseDeltaView() const;
	Vector2f MousePosView() const;
	//values are in 0,0=left,top coordinates in pixel units
	Vector2f MouseDeltaPxl() const;
	Vector2f MousePosPxl() const;

	Vector2f ScrollDelta() const;

	//In the context of this structure, "popping" means indicating the event
	//has been handled, and clearing it
	void PopMouse();
	void PopScroll();

	bool HasKeyEvent(KeyEvent key);
	bool PopKeyEvent(KeyEvent key);
	bool PopKey(Key key); //RELEASE_OR_REPEAT event

	Viewport View() { return view; }

	Viewport view;
	Vector2i dimVec;

	using MouseButtons = std::array<bool, 5>;

	MouseButtons mouseButtonsOld, mouseButtonsCur;
	Vector2f mouseOld, mouseCur, scrollAmt;
	bool mousePopped, scrollPopped;
	
	//std::vector<Key> heldKeys;
	std::deque<KeyEvent> keyEvents;
	std::deque<unsigned int> charEvents;

	Time::clock::duration simTime;

	void Step();

	friend class Window;
};

class Window
{
public:
    //Window initialization and cleanup
    Window();
    ~Window();

	void SetView(Viewport view);
	void SetTime(Time::clock::duration simTime);
	Events GetInput();
	void PreDraw();
	void PostDraw();

    magic_ptr<Vector2i> dim;
    
    bool ShouldClose() const;

private:
    GLFWwindow* window;

	Events newEvents;

	friend void cursor_enter_callback(GLFWwindow* window, int entered);
	friend void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	friend void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	friend void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	friend void character_callback(GLFWwindow* window, unsigned int codepoint);
	friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

//print out a message if there are GL errors
void CheckGLError();

#endif
