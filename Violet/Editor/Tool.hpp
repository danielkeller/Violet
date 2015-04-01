#ifndef TOOL_HPP
#define TOOL_HPP

#include "Rendering/VertexData.hpp"
#include "Object.hpp"
#include "Position.hpp"

class Render;
class Events;

class Tool
{
public:
    Tool(Render& r, Position& position);
	void Update(Events& e, Object camera, Object focused);
	void SetTarget(magic_ptr<Transform> target);
    
private:
	Position& position;
	Object x, y, z;
	magic_ptr<Transform> move;
	magic_ptr<Transform> target;
};

#endif
