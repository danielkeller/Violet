#ifndef TOOL_HPP
#define TOOL_HPP

#include "Rendering/VertexData.hpp"
#include "Core/Object.hpp"
#include "Position.hpp"

class Render;
struct Events;
class RenderPasses;

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
