#ifndef TOOL_HPP
#define TOOL_HPP

#include "Rendering/VertexData.hpp"
#include "Object.hpp"
#include "Position.hpp"

class Render;
class Window;

class Tool
{
public:
    Tool(Render& r, Position& position);
	void Update(Window& w, Object camera, Object focused);
	void SetTarget(magic_ptr<Transform> target);
    
private:
	Position& position;
	Object x, y, z;
	magic_ptr<Transform> move;
	magic_ptr<Transform> target;
};

#endif
