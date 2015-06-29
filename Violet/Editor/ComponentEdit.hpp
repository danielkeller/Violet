#ifndef COMPONENT_EDIT_HPP
#define COMPONENT_EDIT_HPP

#include "UI/Elements.hpp"
#include "Core/Object.hpp"
#include "Assets.hpp"

#include <array>

class Persist;
struct Component;

static const int MOD_WIDTH = 30;

class ComponentEditor
{
public:
	ComponentEditor(std::string name, Component& c)
		: name(name), addButton("+", MOD_WIDTH), removeButton("-", MOD_WIDTH), c(c) {}

	void Draw(Persist& persist, Object selected);

protected:
	UI::TextButton addButton, removeButton;
	std::string name;

	Component& c;
    //return true if the component should save
	virtual bool edit(Object selected) { return false; };
	virtual void add(Object selected) {};
	virtual void remove(Object selected) {};
};

class Position;

class PositionEditor : public ComponentEditor
{
public:
	PositionEditor(Position& pos);

private:
	bool edit(Object selected);
	void add(Object selected);

	Position& position;
	UI::FloatEdit xEdit, yEdit, zEdit;
	std::array<float, 3> curAngle;
	std::array<UI::FloatEdit, 3> angleEdit;
	UI::FloatEdit scaleEdit;
};

class Render;

class RenderEditor : public ComponentEditor
{
public:
	RenderEditor(Render& render, Persist&);

	void DrawPicker(Object selected, Persist& persist);
	void ClosePicker();

private:
	bool edit(Object selected);
	void add(Object selected);

	Render& render;
	UI::Button materialButton;
	UI::Button meshButton;

	enum class AssetPicker
	{
		None, Meshes, Materials
	};
	AssetPicker currentPicker;
	void Toggle(AssetPicker clicked);

	ObjAssets meshes;
	MaterialAssets materials;
};

class NarrowPhase;

class CollisionEditor : public ComponentEditor
{
public:
	CollisionEditor(NarrowPhase& narrowPhase, Render& render);
    
private:
    void add(Object selected);
    bool edit(Object selected);
    
	NarrowPhase& narrowPhase;
    Render& render;
    UI::CheckBox debug;
};

class RigidBody;

class RigidBodyEditor : public ComponentEditor
{
public:
	RigidBodyEditor(RigidBody& rigidBody);
private:
	void add(Object selected);
	RigidBody& rigidBody;
};

#endif
