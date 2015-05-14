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

bool Button::Draw(AlignedBox2i box, const std::string& text)
{
	auto mouse = FrameEvents().MousePosPxl().cast<int>();

	DrawBox(box);
	if (active)
		;
	else if (hovered)
	{
		DrawText(to_upper(text), box);
	}
	else
		DrawText(text, box);

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

RadioGroup::RadioGroup(std::vector<TextButton> buttons)
	: buttons(buttons)
{}

bool RadioGroup::Draw(int& selected)
{
	bool ret = false;
	int curButton = 0;
	for (auto& b : buttons)
	{
		if (b.button.Draw(CurLayout().PutSpace({ b.width, LINEH }),
			selected == curButton ? to_upper(b.text) : b.text))
		{
			if (selected != curButton)
			{
				selected = curButton;
				ret = true;
			}
		}
		++curButton;
	}
	return ret;
}
