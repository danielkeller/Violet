#ifndef LAYOUT_HPP
#define LAYOUT_HPP

struct Layout
{
	enum class Dir
	{
		Up, Down, Left, Right
	};
	Dir direction;

	//size that contents needs
	Vector2i filledSize;
	//size that the layout can fill
	Vector2i size;
	//starting position
	Vector2i pos;

	Layout getNext(Dir dir = Dir::Right)
	{
		if (direction == Dir::Up || direction == Dir::Down)
			return{ dir, { 0, 0 }, { size.x(), 0 }, { pos.x(), pos.y() + filledSize.y() } };
		else
			return{ dir, { 0, 0 }, { 0, size.y() }, { pos.x() + filledSize.x(), pos.y() } };
	}

	void putNext(const Layout& l)
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

	static Layout Top(Vector2i box, Dir dir = Dir::Right)
	{
		return{ dir, { 0, 0 }, box, { 0, 0 } };
	}

	Layout getLast(Dir dir = Dir::Right)
	{
		Layout ret = getNext(dir);

		if (direction == Dir::Up || direction == Dir::Down)
			ret.size << size.x(), size.y() - filledSize.y();
		else
			ret.size << size.x() - filledSize.x(), size.y();

		return ret;
	}
};

#endif