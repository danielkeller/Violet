#include "stdafx.h"
#include "Layout.hpp"

using namespace UI;

Vector2i Scooch(Vector2i init, Layout::Dir dir, int distance)
{
	switch (dir)
	{
	case Layout::Dir::Up:
		return{ init.x(), init.y() - distance };
	case Layout::Dir::Down:
		return{ init.x(), init.y() + distance };
	case Layout::Dir::Left:
		return{ init.x() - distance, init.y() };
	case Layout::Dir::Right:
		return{ init.x() + distance, init.y() };
	default: assert(false && "Layout broken");
		return{};
	}
}

bool Parallel(Layout::Dir a, Layout::Dir b)
{
	if (a == Layout::Dir::Up || a == Layout::Dir::Down)
		return b == Layout::Dir::Up || b == Layout::Dir::Down;
	else
		return b == Layout::Dir::Left || b == Layout::Dir::Right;
}

Layout Layout::getNext(Dir dir) const
{
	if (dir == fill) //for completeness
		return{ dir, grow, 0, maxFill - filledSize, 0, Scooch(pos, fill, filledSize) };
	else if (Parallel(dir, fill)) //useful for getLast
		return{ dir, grow, 0, maxFill - filledSize, 0, Scooch(pos, fill, maxFill) };
	else if (dir == grow)
		return{ dir, fill, 0, across, 0, Scooch(pos, fill, filledSize) };
	else
		return{ dir, fill, 0, across, 0, Scooch(Scooch(pos, fill, filledSize), grow, across) };
}

void Layout::putNext(const Layout& l)
{
	filledSize += l.across;
	across = std::max(across, l.maxFill);
}

AlignedBox2i FixBox(AlignedBox2i box)
{
	return{ box.min().cwiseMin(box.max()), box.max().cwiseMax(box.min()) };
}

AlignedBox2i Layout::Box() const
{
	return FixBox({ pos, Scooch(Scooch(pos, fill, maxFill), grow, across) });
}

Layout Layout::getLast(Dir dir) const
{
	Layout ret = getNext(dir);
	ret.across = maxFill - filledSize;
	return ret;
}

Layout Layout::Top(Vector2i box, Dir dir)
{
	switch (dir)
	{
	case Dir::Right:
		return{ dir, Dir::Down, 0, box.x(), box.y(), { 0, 0 } };
	case Dir::Left:
		return{ dir, Dir::Down, 0, box.x(), box.y(), { box.x(), 0 } };
	case Dir::Down:
		return{ dir, Dir::Right, 0, box.y(), box.x(), { 0, 0 } };
	case Dir::Up:
		return{ dir, Dir::Right, 0, box.y(), box.x(), { 0, box.y() } };
	default: assert(false && "Layout broken");
		return{};
	}

}

LayoutStack::LayoutStack(Vector2i box, Dir dir)
{
	stack.push_back(Layout::Top(box, dir));
}

void LayoutStack::PushLayer(Dir dir /*, int z*/)
{
	stack.push_back(Layout::Top(stack[0].Box().sizes(), dir));
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
	auto& b = stack.back();
	Layout l = b.getNext(b.grow);

	if (l.fill == Dir::Down || l.fill == Dir::Up)
	{
		l.across = size.x();
		l.maxFill = size.y();
	}
	else
	{
		l.across = size.y();
		l.maxFill = size.x();
	}
	b.putNext(l);
	return l;
}

Layout LayoutStack::PutSpace(int advance)
{
	auto& b = stack.back();
	Layout l = b.getNext(b.grow);
	l.across = advance;
	b.putNext(l);
	return l;
}

void LayoutStack::EnsureWidth(int across)
{
	stack.back().across = std::max(stack.back().across, across);
}

Layout LayoutStack::Current() const
{
	return stack.back();
}