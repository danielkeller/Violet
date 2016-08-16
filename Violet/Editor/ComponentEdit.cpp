#include "stdafx.h"
#include "ComponentEdit.hpp"

#include "UI/Text.hpp"
#include "Edit.hpp"
#include "Physics/Collision.hpp"
#include "Physics/RigidBody.hpp"

void ComponentEditor::Draw(Persist& persist, Object selected)
{
	UI::LayoutStack& l = UI::CurLayout();

	l.PushNext(UI::Layout::Dir::Right); l.PutSpace(MOD_WIDTH);
	UI::DrawText(name, l.PutSpace({ LB_WIDTH - 2 * MOD_WIDTH, UI::LINEH }));

	if (c.Has(selected))
	{
		bool doRemove = removeButton.Draw(); l.Pop();

		if (edit(selected))
			c.Save(selected, persist);

		if (doRemove)
		{
			remove(selected);
			c.Remove(selected);
			c.Save(selected, persist);
		}
	}
	else
	{
		if (addButton.Draw())
		{
			add(selected);
			c.Save(selected, persist);
		}
		l.Pop();
	}
}

PositionEditor::PositionEditor(Position& pos)
	: ComponentEditor("position", pos), position(pos)
	, xEdit(LB_WIDTH / 3), yEdit(LB_WIDTH / 3), zEdit(LB_WIDTH / 3)
	, angleEdit({ LB_WIDTH / 3, LB_WIDTH / 3, LB_WIDTH / 3 })
	, scaleEdit(LB_WIDTH)
{}

//decompose rot into rotation around axis (twist) and rotation perpendicular (swing)
//rot = twist * swing
std::tuple<Quaternionf, Quaternionf> TwistSwing(const Quaternionf& rot, const Vector3f& axis)
{
	auto proj = rot.vec().dot(axis) * axis;
	Quaternionf twist{ rot.w(), proj.x(), proj.y(), proj.z() };
	twist.normalize();
	return std::make_tuple(twist, twist.conjugate() * rot);
}

bool PositionEditor::edit(Object selected)
{
	UI::LayoutStack& l = UI::CurLayout();

	Transform xfrm = *position[selected];

	bool stopped = false, moved = false;

	l.PushNext(UI::Layout::Dir::Right);
	stopped |= xEdit.Draw(xfrm.pos.x());
	stopped |= yEdit.Draw(xfrm.pos.y());
	stopped |= zEdit.Draw(xfrm.pos.z());
	moved |= xEdit.editing | yEdit.editing | zEdit.editing;
	l.Pop();

	UI::DrawText("rotation", l.PutSpace({ LB_WIDTH, UI::LINEH }));
	l.PushNext(UI::Layout::Dir::Right);

	//the angles displayed here are how much the object appears to be rotated when
	//looking down the given axis at it. The twist-swing decomposition allows us
	//to separate out the rest of the rotation and edit just the apparent rotation
	//TODO: radian/degree
	for (int axis = 0; axis < 3; ++axis)
	{
		stopped |= angleEdit[axis].Draw(curAngle[axis]);
		moved |= angleEdit[axis].editing;

		Quaternionf swing;
		Quaternionf twist;
		std::tie(twist, swing) = TwistSwing(xfrm.rot, Vector3f::Unit(axis));
		if (angleEdit[axis].editing) //editor in control
		{
			xfrm.rot = Eigen::AngleAxisf(curAngle[axis], Vector3f::Unit(axis)) * swing;
		}
		else //object in control
		{
			Vector3f other = Vector3f::Unit((axis + 1) % 3);
			Vector3f rotated = twist * other;
			curAngle[axis] = std::atan2(rotated[(axis + 2) % 3], rotated[(axis + 1) % 3]);
		}
	}

	l.Pop();

	//xfrm.rot.normalize(); //?

	UI::DrawText("scale", l.PutSpace({ LB_WIDTH, UI::LINEH }));
	stopped |= scaleEdit.Draw(xfrm.scale);
	moved |= scaleEdit.editing;

	//track the position
	if (moved)
		position[selected].set(xfrm);

	//save the object once we stop editing
	return stopped;
}

void PositionEditor::add(Object selected)
{
	position.Set(selected, {});
}

RenderEditor::RenderEditor(Render& render)
	: ComponentEditor("rendering", render), render(render)
	, currentPicker(AssetPicker::None)
{}

void RenderEditor::DrawPicker(Object selected, Persist& persist)
{
    if (selected == Object::none)
        currentPicker = AssetPicker::None;
    
    if (currentPicker != AssetPicker::None)
	{
		auto tup = render.Info(selected);
		auto old = tup;

		if (currentPicker == AssetPicker::Meshes
			? assets.DrawObj(std::get<1>(tup))
			: assets.DrawMat(std::get<0>(tup), persist))
			currentPicker = AssetPicker::None;

		if (old != tup)
		{
			render.Remove(selected);
			render.Create(selected, tup);
			render.Save(selected, persist);
		}
	}
}

void RenderEditor::ClosePicker()
{
    assets.Close();
}

void RenderEditor::Toggle(AssetPicker clicked)
{
	if (currentPicker == clicked)
		assets.Close();
	else currentPicker = clicked;
}

bool RenderEditor::edit(Object selected)
{
	UI::LayoutStack& l = UI::CurLayout();

	auto tup = render.Info(selected);
	UI::AlignedBox2i title = l.PutSpace({ LB_WIDTH, UI::LINEH });
	UI::AlignedBox2i name = l.PutSpace({ LB_WIDTH, UI::LINEH });

	UI::DrawText("material", title);
	UI::DrawText(std::get<0>(tup).Name(), name);
	if (materialButton.Draw(title.extend(name)))
		Toggle(AssetPicker::Materials);

	title = l.PutSpace({ LB_WIDTH, UI::LINEH });
	name = l.PutSpace({ LB_WIDTH, UI::LINEH });

	UI::DrawText("mesh", title);
	UI::DrawText(std::get<1>(tup).Name(), name);
	if (meshButton.Draw(title.extend(name)))
		Toggle(AssetPicker::Meshes);

	if (std::get<2>(tup) == Mobilty::Yes)
		UI::DrawText("mobile", l.PutSpace({ LB_WIDTH, UI::LINEH }));
	else
		UI::DrawText("not mobile", l.PutSpace({ LB_WIDTH, UI::LINEH }));

	return false;
}

void RenderEditor::add(Object selected)
{
	render.Create(selected, Material{}, "assets/capsule.obj");
}

CollisionEditor::CollisionEditor(Collision& collision, Render& render)
	: ComponentEditor("collision", collision), collision(collision), render(render)
    , debug("debug view", LB_WIDTH)
{}

void CollisionEditor::add(Object selected)
{
	auto tup = render.Info(selected);
	collision.Add(selected, std::get<1>(tup).Name());
}

bool CollisionEditor::edit(Object)
{
    debug.Draw(collision.Debug());
    return false;
}

RigidBodyEditor::RigidBodyEditor(RigidBody& rigidBody)
	: ComponentEditor("rigid body", rigidBody), rigidBody(rigidBody)
    , paused("pause", LB_WIDTH/2), debug("debug view", LB_WIDTH/2)
{}

void RigidBodyEditor::add(Object selected)
{
	rigidBody.Add(selected, 1, 1);
}

bool RigidBodyEditor::edit(Object)
{
    UI::CurLayout().PushNext(UI::Layout::Dir::Right);
    debug.Draw(rigidBody.Debug());
    paused.Draw(rigidBody.paused);
    UI::CurLayout().Pop();
    return false;
}
