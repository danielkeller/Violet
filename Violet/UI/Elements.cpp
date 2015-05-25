#include "stdafx.h"
#include "Elements.hpp"
#include "Window.hpp"
#include "Layout.hpp"
#include "PixelDraw.hpp"
#include "Text.hpp"

using namespace UI;

Focusable::Focusable()
	: focused(false), tabbedIn(false)
{}

bool Focusable::anyFocused = false;

bool Focusable::Draw(AlignedBox2i box)
{
	auto& events = FrameEvents();
	auto mouse = events.MousePosPxl().cast<int>();
	bool ret = false;
	tabbedIn = false;

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
	if (focused && events.HasKeyEvent({ { GLFW_KEY_TAB, 0 }, RELEASE_OR_REPEAT }))
	{
		focused = false;
		ret = true;
	}

	//catch focus
	if (!anyFocused && events.PopKeyEvent({ { GLFW_KEY_TAB, 0 }, RELEASE_OR_REPEAT }))
	{
		focused = true;
		tabbedIn = true;
	}

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

Color Button::GetColor() const
{
	return active ? Colors::secondary :
		hovered ? Colors::divider : Colors::bg;
}

bool Button::Draw(AlignedBox2i box)
{
	return Draw(box, "");
}

bool Button::Draw(AlignedBox2i box, const std::string& text)
{
	return Draw(box, text, GetColor());
}

bool Button::Draw(AlignedBox2i box, const std::string& text, Color color)
{
	DrawText(text, box);
	DrawBox(box, color, 0);

	return Behavior(box);
}

bool Button::Behavior(AlignedBox2i box)
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

	//while hovered we handle mouse events
	if (hovered)
		FrameEvents().PopMouse();

	return false;
}

TextButton::TextButton(std::string text, int width)
	: width(width), text(text)
{}

bool TextButton::Draw()
{
	Layout l = CurLayout().PutSpace({ width, LINEH });
	return button.Draw(l, text);
}

ModalBoxRAII::ModalBoxRAII(UI::Layout::Dir dir)
	: closed(false)
{
	UI::PushModal();
	Events& e = UI::FrameEvents();
	//FIXME: this steals the event
	if (e.PopKeyEvent({ { GLFW_KEY_ESCAPE, 0 }, GLFW_PRESS }))
		closed = true;

	UI::LayoutStack& l = UI::CurLayout();
	l.PushLayer(dir);
	UI::PushZ(10); //I don't like this

	l.Inset(30);
	UI::DrawBox(l.Current());
	UI::DrawShadow(l.Current());
	l.Inset(20);
}

ModalBoxRAII::~ModalBoxRAII()
{
	UI::PopZ();
	UI::CurLayout().PopLayer();
	UI::PopModal();
}

Animation::Animation()
	: start(Time::clock::duration::zero())
	, last(Time::clock::duration::zero())
{}

bool UI::Animation::Running() const
{
	return UI::FrameEvents().simTime == last + Time::dt
		|| UI::FrameEvents().simTime == last;
}

int Animation::Run(int initial, int final, Ease ease, Time::clock::duration time)
{
	auto simTime = UI::FrameEvents().simTime;

	//restart
	if (!Running())
		last = start = simTime;
	else
		last = simTime;
	
	//must be float since it's fractional
	auto alpha = std::chrono::duration_cast<millifloat>(simTime - start)
		/ std::chrono::duration_cast<millifloat>(time);
	if (alpha >= 1.f)
		return final;

	//1d cubic bezier control points (the curve is from 0 to 1)
	float x1, x2;
	if (ease == Ease::In) std::tie(x1, x2) = std::make_tuple(.4f, 1.f);
	if (ease == Ease::Out) std::tie(x1, x2) = std::make_tuple(0.f, .6f);
	if (ease == Ease::InOut) std::tie(x1, x2) = std::make_tuple(.4f, .6f);

	float x = 3 * (1-alpha) * alpha * ((1-alpha) * x1 + alpha*x2) + alpha*alpha*alpha;

	return initial + int((final - initial)*x);
}
