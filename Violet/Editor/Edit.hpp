#ifndef EDIT_HPP
#define EDIT_HPP

#include "Rendering/Picker.hpp"
#include "Editor/Tool.hpp"
#include <unordered_set>

class Mobile;
class Window;
class Render;

class Edit
{
public:
    Edit(Render& r, Window& w, Mobile& m);

    void Editable(Object o);

    void PhysTick();
    void DrawTick();

private:
	Render& r;
    Window& w;
    Mobile& m;
    Picker pick;
    Tool tool;

    std::unordered_set<Object> editable;

    Object focused;
    Object selected;
    bool mouseDown;
};

#endif
