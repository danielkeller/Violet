#ifndef LAYOUT_HPP
#define LAYOUT_HPP

//TODO: Comment on how this all works

namespace UI
{
	using AlignedBox2i = Eigen::AlignedBox2i;

	static const int LINEH = 16;
	//distance from bottom of line to text baseline
	static const int BASELINE_HEIGHT = 4;
	//distance from side of container to edge of text
	static const int TEXT_PADDING = 4;
    //distance between boxes in grid
    static const int GRID_SPACING = 8;
    //size of checkbox
    static const int CHECK_SIZE = 8;
    
    class GridLayout;

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

		Layout getNext(Dir dir) const;
		void putNext(const Layout& l);
		Layout getLast(Dir dir) const;

		AlignedBox2i Box() const;

		operator AlignedBox2i() const { return Box(); }

		static Layout Top(AlignedBox2i box, Dir dir);
	};

	class LayoutStack
	{
	public:
		using Dir = Layout::Dir;
		LayoutStack(AlignedBox2i box, Dir dir = Dir::Right);
        void PushLayer(Dir dir /*, int z*/);
        void PushLayer(AlignedBox2i box, Dir dir/*, int z*/);
		void PopLayer();
		//pushes a new layout, filling in direction dir
		void PushNext(Dir dir);
		//pops the current layout, and pushes the remaining space
		//I think this function is bad because it breaks the stack
		//void PushRest(Dir dir);
		//pops the current layout, and returns the remaining space
		Layout Pop(Dir dir = Dir::Right);
		//adds a box and returns the space it's in
		Layout PutSpace(Vector2i size);
		//adds spacing in the current layout direction and returns the space it's in
		//advance may be negative
		Layout PutSpace(int advance);
		//makes sure the current layout can fit at least width across
		void EnsureWidth(int across);
		//Insets the current layout by amount pixels
		void Inset(int amount);
		//look at the current layout
		Layout Current() const;
        //Start a grid layout. 'dir' is the direction of the first box
        GridLayout StartGrid(int boxSize, Layout::Dir dir);

	private:
		std::vector<Layout> stack;
	};
    
    class GridLayout
    {
    public:
        //Sets the current layout in the stack to the next grid item
        //anything pushed on the layout stack must be popped before
        //moving on to the next grid box
        void Next(Layout::Dir dir);
        //exits the grid layout
        ~GridLayout();
        
    private:
        GridLayout(LayoutStack& stack, int boxSize);
        
        int x, y;
        const AlignedBox2i box;
        const int boxSize;
        LayoutStack& stack;
        friend class LayoutStack;
    };
}

#endif