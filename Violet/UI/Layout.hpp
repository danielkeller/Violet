#ifndef LAYOUT_HPP
#define LAYOUT_HPP

namespace UI
{
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

		Layout getNext(Dir dir = Dir::Right);
		void putNext(const Layout& l);
		//static Layout Box(Vector2i box);
		Layout getLast(Dir dir = Dir::Right);

		static Layout Top(Vector2i box, Dir dir = Dir::Right);
	};

	class LayoutStack
	{
	public:
		using Dir = Layout::Dir;
		LayoutStack(Vector2i box, Dir dir = Dir::Right);
		void PushLayer(Dir dir = Dir::Right /*, int z*/);
		//pushes a new layout, filling in direction dir
		void PushNext(Dir dir = Dir::Right);
		//pops the current layout, and pushes the remaining space
		void PushRest(Dir dir = Dir::Right);
		//pops the current layout, and returns the remaining space
		Layout Pop(Dir dir = Dir::Right);
		//adds a box and returns the space it's in
		Layout PutSpace(Vector2i size);

	private:
		std::vector<Layout> stack;
	};
}

#endif