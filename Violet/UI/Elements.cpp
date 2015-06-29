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
	Vector2i mouse = events.MousePosSc().cast<int>();
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
	Vector2i mouse = FrameEvents().MousePosSc().cast<int>();

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

	//the button is active if it's hovered and the mouse is down on it
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

CheckBox::CheckBox(std::string text, int width)
    : width(width), text(text)
{}

void CheckBox::Draw(bool& checked)
{
    LayoutStack& l = CurLayout();
    l.PushNext(Layout::Dir::Right); //whole area
    l.EnsureWidth(LINEH);
    
    int pad = (LINEH - CHECK_SIZE)/2;
    l.PutSpace(pad); //left pad
    l.PushNext(Layout::Dir::Down);
    l.PutSpace(pad); //top pad
    Layout checkArea = l.PutSpace({CHECK_SIZE, CHECK_SIZE});
    
    PushZ();
    if (checked)
        DrawBox(checkArea, Colors::selection, Colors::selection);
    else
        DrawBox(checkArea, 0, Colors::divider);
    PopZ();
    
    l.Pop();
    l.PutSpace(pad); //right pad
    
    Layout textArea = l.PutSpace(width - LINEH);
    DrawText(text, textArea);
    
    Layout mainArea = l.Current();
    mainArea.maxFill = width; //shrink it down to the actual width
    DrawBox(mainArea, button.GetColor(), 0);
    
    l.Pop();
    
    if (button.Behavior(mainArea))
        checked = !checked;
}

Animation::Animation(Ease ease, Time::clock::duration time)
	: ease(ease), time(time), start(-1s), last(-1s)
{}

bool Animation::Started() const
{
	return UI::FrameEvents().simTime == last + Time::dt
		|| UI::FrameEvents().simTime == last;
}

void Animation::Start()
{
	if (!Started())
		last = start = UI::FrameEvents().simTime;
}

float Animation::Continue() const
{
	if (!Started())
		return 0.f;

	auto simTime = UI::FrameEvents().simTime;
	last = simTime;

	//must be float since it's fractional
	auto alpha = std::chrono::duration_cast<millifloat>(simTime - start)
		/ std::chrono::duration_cast<millifloat>(time);
	if (alpha >= 1.f)
		return 1.f;

	//1d cubic bezier control points (the curve is from 0 to 1)
	float x1 = 0.f, x2 = 0.f;
	if (ease == Ease::In) std::tie(x1, x2) = std::make_tuple(.4f, 1.f);
	if (ease == Ease::Out) std::tie(x1, x2) = std::make_tuple(0.f, .6f);
	if (ease == Ease::InOut) std::tie(x1, x2) = std::make_tuple(.4f, .6f);

	return 3 * (1 - alpha) * alpha * ((1 - alpha) * x1 + alpha*x2) + alpha*alpha*alpha;
}

float Animation::Run()
{
	Start();
	return Continue();
}

SlideInOut::SlideInOut(Time::clock::duration time)
	: open(Ease::In, time), close(Ease::Out, time)
{}

bool SlideInOut::Draw(int size)
{
	UI::LayoutStack& l = UI::CurLayout();
	l.PutSpace(int((open.Run() - 1.f) * size));
	int closing = int(-close.Continue() * size);
	l.PutSpace(closing);
	return closing == -size;
}

void SlideInOut::Close()
{
	close.Start();
}

ModalBox::ModalBox()
	: open(Ease::In, 200ms), close(Ease::Out, 200ms)
{}

ModalBox::RAII ModalBox::Draw(UI::Layout::Dir dir, AlignedBox2i initBox)
{
	UI::PushModal();

	UI::LayoutStack& l = UI::CurLayout();
	l.PushLayer(dir);
	UI::PushZ(10);
	l.Inset(30);

	float anim = open.Run() - close.Continue();

	AlignedBox2i final = l.Current();
	AlignedBox2i box = Eigen::AlignedBox2f{
		initBox.min().cast<float>() * (1.f - anim) + final.min().cast<float>()*anim,
		initBox.max().cast<float>() * (1.f - anim) + final.max().cast<float>()*anim }.cast<int>();

	UI::DrawBox(box);
	UI::DrawShadow(box);
	l.Inset(20);

	return{ *this };
}

bool ModalBox::Closed() const
{
	return close.Continue() == 1.f;
}

bool ModalBox::Ready() const
{
	return open.Continue() - close.Continue() == 1.f;
}

ModalBox::RAII::~RAII()
{
	if (UI::FrameEvents().PopKeyEvent({ { GLFW_KEY_ESCAPE, 0 }, GLFW_PRESS }))
		box.close.Start();

	UI::PopZ();
	UI::CurLayout().PopLayer();
	UI::PopModal();
}