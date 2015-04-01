#ifndef EDIT_HPP
#define EDIT_HPP

#include "Editor/Tool.hpp"
#include <unordered_set>

class Render;
class RenderPasses;
class Persist;
class Events;

class Edit
{
public:
	Edit(Render& r, RenderPasses& rp, Position& position);

    void Editable(Object o);

	void PhysTick(Events& e, Object camera);

private:
	RenderPasses& rp;
	Position& position;
    Tool tool;

    std::unordered_set<Object> editable;

    Object focused;
    Object selected;
	float viewPitch, viewYaw;
};

#endif
