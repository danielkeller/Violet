#ifndef EDIT_HPP
#define EDIT_HPP

#include "Rendering/Picker.hpp"
#include "Editor/Tool.hpp"
#include <unordered_set>

class Window;
class Render;
class RenderPasses;
class Persist;

class Edit
{
public:
	Edit(Render& r, RenderPasses& rp, Window& w, Position& position);

    void Editable(Object o);

    void PhysTick(Object camera);

private:
    Window& w;
	Picker& pick;
	RenderPasses& rp;
	Position& position;
    Tool tool;

    std::unordered_set<Object> editable;

    Object focused;
    Object selected;
    bool mouseDown;
	float viewPitch, viewYaw;
};

#endif
