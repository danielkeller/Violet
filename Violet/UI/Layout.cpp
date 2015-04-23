#include "stdafx.h"
#include "Layout.hpp"

using namespace UI;

Layout Layout::getNext(Dir dir)
{
	if (direction == Dir::Up || direction == Dir::Down)
		return{ dir, { 0, 0 }, { size.x(), 0 }, { pos.x(), pos.y() + filledSize.y() } };
	else
		return{ dir, { 0, 0 }, { 0, size.y() }, { pos.x() + filledSize.x(), pos.y() } };
}

void Layout::putNext(const Layout& l)
{
	if (direction == Dir::Up || direction == Dir::Down)
	{
		filledSize.y() += l.size.y();
		size.x() = filledSize.x() = std::max(filledSize.x(), l.size.x());
	}
	else
	{
		filledSize.x() += l.size.x();
		size.y() = filledSize.y() = std::max(filledSize.y(), l.size.y());
	}
}
/*
static Layout Box(Vector2i box)
{
return{ Dir::Right, box, box };
}*/

Layout Layout::Top(Vector2i box, Dir dir)
{
	return{ dir, { 0, 0 }, box, { 0, 0 } };
}

Layout Layout::getLast(Dir dir)
{
	Layout ret = getNext(dir);

	if (direction == Dir::Up || direction == Dir::Down)
		ret.size << size.x(), size.y() - filledSize.y();
	else
		ret.size << size.x() - filledSize.x(), size.y();

	return ret;
}

LayoutStack::LayoutStack(Vector2i box, Dir dir)
{
	stack.push_back(Layout::Top(box, dir));
}

void LayoutStack::PushLayer(Dir dir /*, int z*/)
{
	stack.push_back(Layout::Top(stack[0].size, dir));
}

void LayoutStack::PushNext(Dir dir)
{
	Layout l = stack.back().getNext(dir);
	stack.push_back(l);
}

void LayoutStack::PushRest(Dir dir)
{
	Layout l = Pop(dir);
	stack.push_back(l);
}

Layout LayoutStack::Pop(Dir dir)
{
	Layout l = stack.back().getLast(dir);
	Layout popped = stack.back();
	stack.pop_back();
	if (!stack.empty())
		stack.back().putNext(popped);
	return l;
}

Layout LayoutStack::PutSpace(Vector2i size)
{
	Layout l = stack.back().getNext();
	l.size = l.size.cwiseMax(size);
	stack.back().putNext(l);
	return l;
}