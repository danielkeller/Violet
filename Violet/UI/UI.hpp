#ifndef UI_HPP
#define UI_HPP

class Window;
class Events;

namespace UI
{
	class LayoutStack;

	void Draw(Window& w);

	//fixme: thread safety
	void BeginFrame(Window& w, Events e);
	Events& FrameEvents();
	LayoutStack& CurLayout();
}

#endif