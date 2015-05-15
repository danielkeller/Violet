#include "stdafx.h"
#include "Elements.hpp"
#include "Window.hpp"
#include "Layout.hpp"
#include "PixelDraw.hpp"
#include "Text.hpp"

using namespace UI;

std::string to_upper(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	return str;
}

Focusable::Focusable()
	: focused(false)
{}

bool Focusable::anyFocused = false;

bool Focusable::Draw(AlignedBox2i box)
{
	auto& events = FrameEvents();
	auto mouse = events.MousePosPxl().cast<int>();
	bool ret = false;

	if (events.MouseClick(GLFW_MOUSE_BUTTON_LEFT))
	{
		if (box.contains(mouse))
			focused = true;
		else
		{
			ret = focused;
			focused = false;
		}
	}

	//throw focus (leave the event for the catching control)
	if (focused && events.HasKeyEvent({ { GLFW_KEY_TAB, 0 }, GLFW_PRESS }))
	{
		focused = false;
		ret = true;
	}

	//catch focus
	if (!anyFocused && events.PopKeyEvent({ { GLFW_KEY_TAB, 0 }, GLFW_PRESS }))
		focused = true;

	anyFocused |= focused;
	anyFocused &= !ret;

	return ret;
}

void Focusable::Unfocus()
{
	anyFocused = focused = false;
}

Button::Button()
	: hovered(false), active(false)
{}

bool Button::Draw(AlignedBox2i box)
{
	auto mouse = FrameEvents().MousePosPxl().cast<int>();

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

Color Button::GetColor() const
{
	return active ? Colors::secondary :
		hovered ? Colors::divider : Colors::bg;
}

bool Button::Draw(AlignedBox2i box, const std::string& text)
{
	return Draw(box, text, GetColor());
}

bool Button::Draw(AlignedBox2i box, const std::string& text, Color color)
{
	auto mouse = FrameEvents().MousePosPxl().cast<int>();
	DrawText(text, box);

	DrawBox(box, color, 0);

	return Draw(box);
}

TextButton::TextButton(std::string text, int width)
	: width(width), text(text)
{}

bool TextButton::Draw()
{
	Layout l = CurLayout().PutSpace({ width, LINEH });
	return button.Draw(l, text);
}