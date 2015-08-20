#ifndef EDIT_HPP
#define EDIT_HPP

#include "UI/Elements.hpp"
#include "Core/Component.hpp"
#include "ComponentEdit.hpp"
#include "Tool.hpp"

#include <unordered_set>

class Render;
class RenderPasses;
class NarrowPhase;
class BroadPhase;
class RigidBody;
class Persist;
struct Events;

//???
static const int LB_WIDTH = 250;

class Edit : public Component
{
public:
	Edit(Render& r, RenderPasses& rp, Position& position, ObjectName& objName,
		NarrowPhase& narrowPhase, BroadPhase& broadPhase, RigidBody& rigidBody, ComponentManager& mgr,
		Persist& persist);

    void Editable(Object o);

	void PhysTick(Object camera);

private:
	void Load(const Persist&);
	void Unload(const Persist&);
	bool Has(Object) const;
	void Save(Object, Persist&) const;
	void Remove(Object);

	bool enabled;
	RenderPasses& rp;
	Position& position;
	ObjectName& objName;
	ComponentManager& mgr;
	Persist& persist;
    Tool tool;

    Object focused;
    Object selected;
	float viewPitch, viewYaw;

	UI::SlideInOut slide;

	//Name
	std::string curObjectName;
	UI::LineEdit objectNameEdit;

	PositionEditor posEdit;
	RenderEditor renderEdit;
	CollisionEditor collEdit;
	RigidBodyEditor rbEdit;
    
    UI::TextButton addPB;
    BroadPhase& broadPhase;

	//Object
	UI::TextButton newObject, delObject;
	UI::SelectList<Object> objectSelect;
};

MAKE_PERSIST_TRAITS(Edit, Object)

#endif
