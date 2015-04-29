#include "stdafx.h"
#include "Elements.hpp"
#include "Window.hpp"
#include "Layout.hpp"
#include "PixelDraw.hpp"
#include "Text.hpp"

#include <iostream>

using namespace UI;

Button::Button(std::string text)
	: text(text), hovered(false), active(false)
{}

bool Button::Draw()
{
	Layout l = CurLayout().PutSpace({ 80, 20 });
	AlignedBox2i box = l.Box();

	auto mouse = FrameEvents().MousePosPxl().cast<int>();

	DrawBox(box);
	if (active)
		;
	else if (hovered)
	{
		std::string t = text;
		std::transform(t.begin(), t.end(), t.begin(), ::toupper);
		DrawText(t, box);
	}
	else
		DrawText(text, box);

	//the button is clicked if it was active last frame and the mouse was released
	if (active && FrameEvents().MouseRelease(GLFW_MOUSE_BUTTON_LEFT))
	{
		//we handled the mouse up event
		FrameEvents().PopMouse();
		return true;
	}

	//hovered state changes only when the mouse is up
	if (!FrameEvents().MouseButton(GLFW_MOUSE_BUTTON_LEFT))
		hovered = box.contains(mouse);

	//the button is active if it's clicked and hovered
	active = hovered && box.contains(mouse) && FrameEvents().MouseButton(GLFW_MOUSE_BUTTON_LEFT);

	//while active we handle mouse events
	//if (active)
	//	FrameEvents().PopMouse();

	return false;
}