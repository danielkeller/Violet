#ifndef EDIT_HPP
#define EDIT_HPP

#include "Editor/Tool.hpp"
#include <unordered_set>

#include "UI/Elements.hpp"

class Render;
class RenderPasses;
class Persist;
class Events;

class Edit
{
public:
	Edit(Render& r, RenderPasses& rp, Position& position, ObjectName& objName);

    void Editable(Object o);

	void PhysTick(Events& e, Object camera);

private:
	bool enabled;
	RenderPasses& rp;
	Position& position;
	ObjectName& objName;
    Tool tool;

    std::unordered_set<Object> editable;

    Object focused;
    Object selected;
	float viewPitch, viewYaw;

	std::string curObjectName;
	UI::LineEdit objectNameEdit;
	UI::SelectList<Object> objectSelect;

	void Select(Object obj);
};

#endif
