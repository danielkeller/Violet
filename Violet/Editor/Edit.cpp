#include "stdafx.h"
#include "Edit.hpp"
#include "Window.hpp"
#include "Rendering/Render.hpp"
#include "Persist.hpp"

#include "UI/PixelDraw.hpp"
#include "UI/Text.hpp"
#include "UI/Layout.hpp"

#include <iostream>

Edit::Edit(Render& r, RenderPasses& rp, Position& position, ObjectName& objName, Persist& persist)
	: enabled(true)
	, rp(rp), r(r), position(position), objName(objName), persist(persist)
	, tool(r, position)
	, focused(Object::none), selected(Object::none)
	, viewPitch(0), viewYaw(0)
	, objectNameEdit(LB_WIDTH)
	, posEdit("position"), renderEdit("render")
	, xEdit(LB_WIDTH / 3), yEdit(LB_WIDTH / 3), zEdit(LB_WIDTH / 3)
	, angleEdit({ LB_WIDTH / 3, LB_WIDTH / 3, LB_WIDTH / 3 })
	, scaleEdit(LB_WIDTH), objectSelect(LB_WIDTH)
	, newObject("new", MOD_WIDTH)
{
	for (auto o : persist.GetAll<Edit>())
		editable.insert(std::get<0>(o));
}

void Edit::Editable(Object o)
{
    editable.insert(o);
	persist.Set<Edit>(o);
}

//decompose rot into rotation around axis (twist) and rotation perpendicular (swing)
//rot = twist * swing
std::tuple<Quaternionf, Quaternionf> TwistSwing(const Quaternionf& rot, const Vector3f& axis)
{
	auto proj = rot.vec().dot(axis) * axis;
	Quaternionf twist{ rot.w(), proj.x(), proj.y(), proj.z() };
	twist.normalize();
	return std::make_tuple(twist, twist.conjugate() * rot);
}

template<class Component, typename EditTy, typename AddTy, typename RemoveTy>
void Edit::ComponentEditor::Draw(Component& c, Object selected, EditTy edit, AddTy add, RemoveTy remove)
{
	UI::LayoutStack& l = UI::CurLayout();
	l.PushNext(UI::Layout::Dir::Right); l.PutSpace(MOD_WIDTH);
	UI::DrawText(name, l.PutSpace({ LB_WIDTH - 2 * MOD_WIDTH, UI::LINEH }));

	if (c.Has(selected))
	{
		bool doRemove = removeButton.Draw(); l.Pop();

		if (edit())
			c.Save(selected);

		if (doRemove)
		{
			remove();
			c.Save(selected);
		}
	}
	else
	{
		if (addButton.Draw())
		{
			add();
			c.Save(selected);
		}
		l.Pop();
	}
}

void Edit::PhysTick(Events& e, Object camera)
{
	if (e.PopKeyEvent({ { GLFW_KEY_ESCAPE, 0 }, GLFW_RELEASE }))
		enabled = !enabled;

	if (!enabled)
		return;

	Object picked = rp.Pick(e.MousePosPxl());
	Object newSelect = selected;

	if (e.MouseClick(GLFW_MOUSE_BUTTON_LEFT))
    {
		if (picked == Object::none) //click outside to deselect
			newSelect = Object::none;
		else if (editable.count(picked)) //click to select
			newSelect = picked;

		//don't register picks outside of the viewport
		if (picked != Object::invalid)
			focused = picked;
	}
	//save the object once we stop moving
	else if (e.MouseRelease(GLFW_MOUSE_BUTTON_LEFT)
		&& selected != Object::none
		&& selected != focused) //a hack to see if we were dragging the tool
		position.Save(selected);

	//focus reverts to the selectable object that was selected
	if (!e.MouseButton(GLFW_MOUSE_BUTTON_LEFT))
		focused = selected;

	tool.Update(e, camera, focused);

	rp.Highlight(picked, RenderPasses::Hovered);
	rp.Highlight(focused, RenderPasses::Focused);
	rp.Highlight(selected, RenderPasses::Selected);
    
	//right mouse to rotate
    if (e.MouseButton(GLFW_MOUSE_BUTTON_RIGHT))
    {
		auto mdelta = e.MouseDeltaScr();
		viewPitch -= mdelta.x();
		viewYaw += mdelta.y();
		position[camera]->rot = Eigen::AngleAxisf(viewYaw, Vector3f::UnitX())
						      * Eigen::AngleAxisf(viewPitch, Vector3f::UnitZ());
    }

	position[camera]->pos *= (1 - e.ScrollDelta().y()*.05f);
	
	UI::LayoutStack& l = UI::CurLayout() = UI::LayoutStack(e.dimVec, UI::Layout::Dir::Left);

	//left bar
	l.PushNext(UI::Layout::Dir::Down);
	l.EnsureWidth(LB_WIDTH);

	if (selected != Object::none)
	{
		if (objectNameEdit.Draw(curObjectName))
		{
			objName.Rename(selected, curObjectName);
			//so it comes back alphabetically
			objectSelect.items.erase(selected);
		}

		l.PutSpace(UI::LINEH);

		posEdit.Draw(position, selected,
			[&]()
		{
			Transform xfrm = *position[selected];

			//save the object once we stop editing
			bool moved = false;

			l.PushNext(UI::Layout::Dir::Right);
			moved |= xEdit.Draw(xfrm.pos.x())
				| yEdit.Draw(xfrm.pos.y())
				| zEdit.Draw(xfrm.pos.z());
			l.Pop();

			UI::DrawText("rotation", l.PutSpace({ LB_WIDTH, UI::LINEH }));
			l.PushNext(UI::Layout::Dir::Right);

			//the angles displayed here are how much the object appears to be rotated when
			//looking down the given axis at it. The twist-swing decomposition allows us
			//to separate out the rest of the rotation and edit just the apparent rotation
			for (int axis = 0; axis < 3; ++axis)
			{
				moved |= angleEdit[axis].Draw(curAngle[axis]);

				Quaternionf swing;
				Quaternionf twist;
				std::tie(twist, swing) = TwistSwing(xfrm.rot, Vector3f::Unit(axis));
				if (angleEdit[axis].editing) //editor in control
				{
					xfrm.rot = Eigen::AngleAxisf(curAngle[axis], Vector3f::Unit(axis)) * swing;
				}
				else //object in control
				{
					auto other = Vector3f::Unit((axis + 1) % 3);
					auto rotated = twist * other;
					curAngle[axis] = std::atan2(rotated[(axis + 2) % 3], rotated[(axis + 1) % 3]);
				}
			}

			l.Pop();

			//xfrm.rot.normalize(); //?

			UI::DrawText("scale", l.PutSpace({ LB_WIDTH, UI::LINEH }));
			moved |= scaleEdit.Draw(xfrm.scale);

			if (moved)
			{
				position[selected].set(xfrm);
				tool.SetTarget(position[selected]);
			}

			return moved;
		},
			[&]() {
			tool.SetTarget(position[selected]);
		},
			[&]() {
			position.Remove(selected);
			tool.SetTarget({});
		});

		renderEdit.Draw(r, selected,
			[&]() {
			auto infRow = [&](const char* name, std::string t) {
				UI::DrawText(name, l.PutSpace({ LB_WIDTH, UI::LINEH }));
				UI::DrawText(t, l.PutSpace({ LB_WIDTH, UI::LINEH }));
			};

			auto tup = r.Info(selected);
			infRow("shader", std::get<0>(tup).Name());
			infRow("material", std::get<1>(tup).Name());
			infRow("mesh", std::get<2>(tup).Name());
			if (std::get<3>(tup) == Mobilty::Yes)
				UI::DrawText("mobile", l.PutSpace({ LB_WIDTH, UI::LINEH }));
			else
				UI::DrawText("not mobile", l.PutSpace({ LB_WIDTH, UI::LINEH }));

			return false;
		},
			[&]() {
			r.Create(selected, { "assets/simple" }, { {}, { "assets/capsule.png" } },
			{ "assets/capsule.obj" });
		},
			[&]() {
			r.Remove(selected);
		});

		l.PutSpace(UI::LINEH);
	}

	l.PushNext(UI::Layout::Dir::Left);
	if (newObject.Draw())
	{
		Object n;
		Editable(n);
	}
	UI::DrawText("objects", l.PutSpace(LB_WIDTH - 2 * MOD_WIDTH));
	l.Pop();

	//TODO: not this (n log(n))
	for (Object o : editable)
		if (objectSelect.items.find(o) == objectSelect.items.end())
		{
			auto& name = objName[o];
			auto it = std::lower_bound(objectSelect.items.begin(),
				objectSelect.items.end(), name,
				[](std::pair<Object, std::string>& l, const std::string& r)
				{return l.second < r; });

			objectSelect.items.try_emplace(it, o, name);
		}

	objectSelect.Draw(newSelect);

	UI::DrawBox(l.Current());

	l.Pop();

	//actually change selection at the end so objects have a chance to save when it changes
	if (selected != newSelect)
	{
		selected = newSelect;

		if (selected != Object::none)
		{
			curObjectName = objName[selected];
			if (position.Has(selected))
				tool.SetTarget(position[selected]);
		}
		else
			tool.SetTarget({});
	}

	//fixme
	e.PopMouse();
	e.PopScroll();
}

template<>
const char* PersistSchema<Edit>::name = "edit";
template<>
Columns PersistSchema<Edit>::cols = { "object" };