#ifndef WINDOW_H
#define WINDOW_H

#include "Viewport.hpp"
#include "Core/Time.hpp"

#include "magic_ptr.hpp"
#include <deque>
#include <array>

//for event stuff
#include "GLFW/include/GLFW/glfw3.h"

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

	//values are in Opengl Normalized Device coordinates
	Vector2f MouseDeltaNdc() const;
	Vector2f MousePosNdc() const;
	//values are in 0,0=left,top coordinates in GLFW screen coordinates
	Vector2f MouseDeltaSc() const;
	Vector2f MousePosSc() const;

	Vector2f ScrollDelta() const;
    
    //These events are derived from whichever raw event is appropriate for the platform
    Vector2f UxPan() const;
    float UxZoom() const;

	//In the context of this structure, "popping" means indicating the event
	//has been handled, and clearing it
	void PopMouse();
	void PopScroll();
    void PopZoom();

	bool HasKeyEvent(KeyEvent key);
	bool PopKeyEvent(KeyEvent key);
	bool PopKey(Key key); //RELEASE_OR_REPEAT event

	//Full window
	Viewport view;
    //3D (non-ui) area
    Viewport mainView;

	using MouseButtons = std::array<bool, 5>;

	MouseButtons mouseButtonsOld, mouseButtonsCur;
	Vector2f mouseOld, mouseCur, scrollAmt;
    float zoomAmt;
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

	void SetTime(Time::clock::duration simTime);
	Events GetInput();
	void PreDraw();
	void PostDraw();
    
    bool ShouldClose() const;
    magic_ptr<Viewport> view;

private:
    GLFWwindow* window;

	Events newEvents;

	friend void cursor_enter_callback(GLFWwindow* window, int entered);
	friend void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    friend void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    friend void zoom_callback(GLFWwindow* window, double zoffset);
    friend void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    friend void window_size_callback(GLFWwindow* window, int width, int height);
	friend void character_callback(GLFWwindow* window, unsigned int codepoint);
	friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

//print out a message if there are GL errors
void CheckGLError();

#endif
