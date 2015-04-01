#include "stdafx.h"
#include "UI.hpp"

#include "Layout.hpp"
#include "PixelDraw.hpp"
#include "Window.hpp"

void DrawUI(Window& w)
{
	Layout mainLayout = Layout::Top(*w.dim, Layout::Dir::Left);

	Layout leftBar = mainLayout.getNext(Layout::Dir::Down);
	Layout lbTop = leftBar.getNext();
	lbTop.size = { 100, 200 };
	leftBar.putNext(lbTop);

	DrawBox(lbTop);

	Layout lbRest = leftBar.getLast();
	DrawBox(lbRest);
	mainLayout.putNext(leftBar);
	
	Layout mainBox = mainLayout.getLast(Layout::Dir::Up);
	Layout botBar = mainBox.getNext();
	botBar.size.y() = 100;
	DrawBox(botBar);
}