#ifndef EDIT_HPP
#define EDIT_HPP

#include "UI/Elements.hpp"
#include "Editor/Tool.hpp"

#include <unordered_set>
#include <array>

class Render;
class RenderPasses;
class Persist;
class Events;

class Edit
{
public:
	Edit(Render& r, RenderPasses& rp, Position& position, ObjectName& objName, Persist& persist);

    void Editable(Object o);

	void PhysTick(Events& e, Object camera);

private:
	bool enabled;
	RenderPasses& rp;
	Position& position;
	ObjectName& objName;
	Persist& persist;
    Tool tool;

    std::unordered_set<Object> editable;

    Object focused;
    Object selected;
	float viewPitch, viewYaw;

	static const int LB_WIDTH = 250;
	static const int MOD_WIDTH = 30;

	//Name
	std::string curObjectName;
	UI::LineEdit objectNameEdit;

	//Position
	UI::TextButton addPos, deletePos;
	UI::FloatEdit xEdit, yEdit, zEdit;
	std::array<float,3> curAngle;
	std::array<UI::FloatEdit,3> angleEdit;
	UI::FloatEdit scaleEdit;

	//Object
	UI::TextButton newObject;
	UI::SelectList<Object> objectSelect;
};

MAKE_PERSIST_TRAITS(Edit, Object)

#endif
