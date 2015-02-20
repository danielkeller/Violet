#ifndef EDIT_HPP
#define EDIT_HPP

#include "Rendering/Picker.hpp"
#include "Editor/Tool.hpp"
#include <unordered_set>

class Window;
class Render;

class Edit
{
public:
    Edit(Render& r, Window& w, Position& position);

    void Editable(Object o);

    void PhysTick(Object camera);
    void DrawTick();

private:
	Render& r;
    Window& w;
	Position& position;
    Picker pick;
    Tool tool;

    std::unordered_set<Object> editable;

    Object focused;
    Object selected;
    bool mouseDown;
	float viewPitch, viewYaw;
};

#endif
