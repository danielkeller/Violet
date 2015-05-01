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
	, rp(rp), position(position), objName(objName), persist(persist)
	, tool(r, position)
	, focused(Object::none), selected(Object::none)
	, viewPitch(0), viewYaw(0)
	, objectNameEdit(LB_WIDTH)
	, xEdit(LB_WIDTH / 3), yEdit(LB_WIDTH / 3), zEdit(LB_WIDTH / 3)
	, angleEdit({ LB_WIDTH / 3, LB_WIDTH / 3, LB_WIDTH / 3 })
	, scaleEdit(LB_WIDTH), objectSelect(LB_WIDTH)
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

		Transform xfrm = *position[selected];

		//save the object once we stop editing
		bool moved = false;

		UI::DrawText("position", l.PutSpace({ LB_WIDTH, UI::LINEH }));
		l.PushNext(UI::Layout::Dir::Right);
		moved |= xEdit.Draw(xfrm.pos.x())
			| yEdit.Draw(xfrm.pos.y())
			| zEdit.Draw(xfrm.pos.z());
		l.Pop();

		UI::DrawText("rotation", l.PutSpace({ LB_WIDTH, UI::LINEH }));
		l.PushNext(UI::Layout::Dir::Right);
		//axes.Draw(axis);

		for (int axis = 0; axis < 3; ++axis)
		{
			moved |= angleEdit[axis].Draw(curAngle[axis]);

			Quaternionf swing;
			Quaternionf twist;
			std::tie(twist, swing) = TwistSwing(xfrm.rot, Vector3f::Unit(axis));
			if (angleEdit[axis].editing)
			{
				xfrm.rot = Eigen::AngleAxisf(curAngle[axis], Vector3f::Unit(axis)) * swing;
			}
			else
			{
				auto other = Vector3f::Unit((axis + 1) % 3);
				auto rotated = twist * other;
				curAngle[axis] = std::atan2(rotated[(axis + 2) % 3], rotated[(axis + 1) % 3]);
			}
		}

		l.Pop();

		xfrm.rot.normalize();

		UI::DrawText("scale", l.PutSpace({ LB_WIDTH, UI::LINEH }));
		moved |= scaleEdit.Draw(xfrm.scale);

		position[selected].set(xfrm);
		tool.SetTarget(position[selected]);

		if (moved)
			position.Save(selected);
	}

	l.PutSpace(UI::LINEH);

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