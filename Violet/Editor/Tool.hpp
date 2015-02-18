#ifndef TOOL_HPP
#define TOOL_HPP

#include "Rendering/VertexData.hpp"
#include "Object.hpp"
#include "Mobile.hpp"

class Render;
class Window;

class Tool
{
public:
    Tool(Render& r, Mobile& m);
	void Update(Window& w, Object focused);
	magic_ptr<Transform>& Move();
    
private:
	magic_ptr<Transform> CreateArrows(Render& r);

	const Mobile& m;
	Object x, y, z;
	magic_ptr<Transform> move;
};

#endif
