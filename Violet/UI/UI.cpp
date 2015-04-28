#include "stdafx.h"
#include "UI.hpp"

#include "Layout.hpp"
#include "PixelDraw.hpp"
#include "Window.hpp"
#include "Layout.hpp"
#include "Elements.hpp"

#include <iostream>

using namespace UI;

AlignedBox2i UI::Draw(Window& w)
{
	static bool uiOn = true;

	if (FrameEvents().PopKeyEvent({ { GLFW_KEY_ESCAPE, 0 }, GLFW_RELEASE }))
		uiOn = !uiOn;

	if (!uiOn)
		return{ Vector2i{ 0, 0 }, *w.dim };

	LayoutStack& l = CurLayout() = LayoutStack(*w.dim, Layout::Dir::Left);

	//left bar
	l.PushNext(Layout::Dir::Down);
	Layout lbTop = l.PutSpace({ 100, 200 });

	DrawBox(lbTop);

	static Button b;
	b.text = "Button!";
	if (b.Draw())
		std::cerr << "Button!\n";

	Layout lbRest = l.Pop();
	DrawBox(lbRest);
	
	l.PushRest(Layout::Dir::Up);
	Layout botBar = l.PutSpace(100);
	DrawBox(botBar);

	return l.Pop().Box();
}