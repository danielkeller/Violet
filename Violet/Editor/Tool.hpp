#ifndef TOOL_HPP
#define TOOL_HPP

#include "VertexData.hpp"
#include "Object.hpp"
#include "Mobile.hpp"

class Render;
class Window;

class Tool
{
public:
    Tool(Render& r, Mobile& m);
    void Update(Window& w, Object focused);
    
private:
    std::vector<Render::LocationProxy> CreateArrows(Render& r);
    
    Object x, y, z;
    Mobile::MoveProxy move;
    const Mobile& m;
};

#endif
