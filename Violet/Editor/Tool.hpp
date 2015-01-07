#ifndef TOOL_HPP
#define TOOL_HPP

#include "VertexData.hpp"
#include "Object.hpp"
#include "Mobile.hpp"

class Render;
class Picker;
class Time;

class Tool
{
public:
    Tool(Render& r, Picker& pick, Mobile& m);
    void Update(Time& t);
    
private:
    std::vector<Render::LocationProxy> CreateArrows(Render& r);
    
    Object x, y, z;
    Mobile::MoveProxy move;
};

#endif
