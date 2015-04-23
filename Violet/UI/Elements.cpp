#include "stdafx.h"
#include "Elements.hpp"
#include "UI.hpp"
#include "Window.hpp"
#include "Layout.hpp"
#include "PixelDraw.hpp"
#include "Text.hpp"

#include <iostream>

using namespace UI;

bool Button::Draw()
{
	Vector2i size = { 60, 20 };
	Layout l = CurLayout().PutSpace(size);
	Eigen::AlignedBox2i box{ l.pos, l.pos + size };

	auto mouse = FrameEvents().MousePosPxl().cast<int>();

	DrawBox(box);
	if (active)
		;
	else if (hovered)
	{
		std::string t = text;
		std::transform(t.begin(), t.end(), t.begin(), ::toupper);
		DrawText(t, l.pos + Vector2i{ 3, 16 });
	}
	else
		DrawText(text, l.pos + Vector2i{ 3, 16 });

	//the button is clicked if it was active last frame and the mouse was released
	if (active && FrameEvents().MouseRelease(MOUSE_BUTTON_LEFT))
	{
		//we handled the mouse event
		FrameEvents().PopMouse();
		return true;
	}

	//hovered state does not change when the mouse is down
	if (!FrameEvents().MouseButton(MOUSE_BUTTON_LEFT))
		hovered = box.contains(mouse);

	//the button is active if it's clicked and hovered
	active = hovered && box.contains(mouse) && FrameEvents().MouseButton(MOUSE_BUTTON_LEFT);

	return false;
}