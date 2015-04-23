#include "stdafx.h"
#include "UI.hpp"

#include "Layout.hpp"
#include "PixelDraw.hpp"
#include "Window.hpp"
#include "Layout.hpp"
#include "Elements.hpp"

#include <iostream>

using namespace UI;

void UI::Draw(Window& w)
{
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
	Layout botBar = l.PutSpace({ 0, 100 });
	DrawBox(botBar);
}

void UI::BeginFrame(Window& w, Events e)
{
	CurLayout() = LayoutStack(*w.dim);
	FrameEvents() = e;
}

Events& UI::FrameEvents()
{
	static Events events;
	return events;
}

LayoutStack& UI::CurLayout()
{
	static LayoutStack layout({ 0, 0 });
	return layout;
}