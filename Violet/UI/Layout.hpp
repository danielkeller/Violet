#ifndef LAYOUT_HPP
#define LAYOUT_HPP

namespace UI
{
	using AlignedBox2i = Eigen::AlignedBox2i;

	static const int LINEH = 16;
	//distance from bottom of line to text baseline
	static const int BASELINE_HEIGHT = 4;
	//distance from left of container to start of text
	static const int TEXT_LEFT_PAD = 4;

	struct Layout
	{
		enum class Dir
		{
			Up, Down, Left, Right
		};

		Dir fill;
		Dir grow;

		//size that contents needs in fill direction
		int filledSize;
		//size that the layout can fill
		int maxFill;
		//size perpendicular to fill
		int across;
		//starting position
		Vector2i pos;

		Layout getNext(Dir dir = Dir::Right) const;
		void putNext(const Layout& l);
		Layout getLast(Dir dir = Dir::Right) const;

		AlignedBox2i Box() const;

		operator AlignedBox2i() const { return Box(); }

		static Layout Top(Vector2i box, Dir dir = Dir::Right);
	};

	class LayoutStack
	{
	public:
		using Dir = Layout::Dir;
		LayoutStack(Vector2i box, Dir dir = Dir::Right);
		void PushLayer(Dir dir = Dir::Right /*, int z*/);
		void PopLayer();
		//pushes a new layout, filling in direction dir
		void PushNext(Dir dir = Dir::Right);
		//pops the current layout, and pushes the remaining space
		//I think this function is bad because it breaks the stack
		//void PushRest(Dir dir = Dir::Right);
		//pops the current layout, and returns the remaining space
		Layout Pop(Dir dir = Dir::Right);
		//adds a box and returns the space it's in
		Layout PutSpace(Vector2i size);
		//adds spacing in the current layout direction and returns the space it's in
		Layout PutSpace(int advance);
		//makes sure the current layout can fit at least width across
		void EnsureWidth(int across);
		//Insets the current layout by amount pixels
		void Inset(int amount);
		//look at the current layout
		Layout Current() const;

	private:
		std::vector<Layout> stack;
	};
}

#endif