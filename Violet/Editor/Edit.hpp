#ifndef EDIT_HPP
#define EDIT_HPP

#include "UI/Elements.hpp"
#include "Editor/Tool.hpp"
#include "Assets.hpp"

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
	Render& r;
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

	struct ComponentEditor
	{
		ComponentEditor(std::string name)
			: name(name), addButton("+", MOD_WIDTH), removeButton("-", MOD_WIDTH) {}
		UI::TextButton addButton, removeButton;
		std::string name;

		template<class Component, typename EditTy, typename AddTy, typename RemoveTy>
		void Draw(Component& c, Object selected, EditTy edit, AddTy add, RemoveTy remove);
	};

	bool meshesOpen;
	ObjAssets meshes;

	//Name
	std::string curObjectName;
	UI::LineEdit objectNameEdit;

	//Position
	ComponentEditor posEdit;
	ComponentEditor renderEdit;
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
