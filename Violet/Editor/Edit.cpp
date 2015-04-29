#include "stdafx.h"
#include "Edit.hpp"
#include "Window.hpp"
#include "Rendering/Render.hpp"
#include "Persist.hpp"

#include "UI/PixelDraw.hpp"
#include "UI/Text.hpp"
#include "UI/Layout.hpp"

#include <iomanip>

Edit::Edit(Render& r, RenderPasses& rp, Position& position, ObjectName& objName)
	: enabled(true)
	, rp(rp), position(position), objName(objName)
	, tool(r, position)
	, focused(Object::none), selected(Object::none)
	, viewPitch(0), viewYaw(0)
{
}

void Edit::Editable(Object o)
{
    editable.insert(o);
}

std::string short_string(float val)
{
	std::stringstream ss;
	ss << std::fixed << std::setprecision(3) << val;
	return ss.str();
}

void Edit::PhysTick(Events& e, Object camera)
{
	if (e.PopKeyEvent({ { GLFW_KEY_ESCAPE, 0 }, GLFW_RELEASE }))
		enabled = !enabled;

	if (!enabled)
		return;

	Object picked = rp.Pick(e.MousePosPxl());

	if (e.MouseClick(GLFW_MOUSE_BUTTON_LEFT))
    {
		if (picked == Object::none) //click outside to deselect
			Select(Object::none);
		else if (editable.count(picked)) //click to select
			Select(picked);

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
	static const int LB_WIDTH = 250;
	l.PushNext(UI::Layout::Dir::Down);
	l.EnsureWidth(LB_WIDTH);

	if (selected != Object::none)
	{
		objectNameEdit.width = LB_WIDTH;

		if (objectNameEdit.Draw(curObjectName))
			objName.Rename(selected, curObjectName);
		
		l.PutSpace(UI::LINEH);

		Transform xfrm = *position[selected];

		auto numbercol = [&](int cols, float num) {
			UI::DrawText(short_string(num), l.PutSpace({ LB_WIDTH / cols, UI::LINEH }));
		};

		UI::DrawText("position", l.PutSpace({ LB_WIDTH, UI::LINEH }));
		l.PushNext(UI::Layout::Dir::Right);
		numbercol(3, xfrm.pos.x());
		numbercol(3, xfrm.pos.y());
		numbercol(3, xfrm.pos.z());
		l.Pop();

		UI::DrawText("rotation", l.PutSpace({ LB_WIDTH, UI::LINEH }));
		l.PushNext(UI::Layout::Dir::Right);
		numbercol(4, xfrm.rot.w());
		numbercol(4, xfrm.rot.x());
		numbercol(4, xfrm.rot.y());
		numbercol(4, xfrm.rot.z());
		l.Pop();

		UI::DrawText("scale", l.PutSpace({ LB_WIDTH, UI::LINEH }));
		numbercol(1, xfrm.scale);
	}

	l.PutSpace(UI::LINEH);

	objectSelect.width = LB_WIDTH;

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

	Object slPicked = selected;
	objectSelect.Draw(slPicked);
	Select(slPicked);

	UI::DrawBox(l.Current());

	l.Pop();

	//fixme
	e.PopMouse();
	e.PopScroll();
}

void Edit::Select(Object obj)
{
	if (selected == obj)
		return;

	selected = obj;

	if (selected != Object::none)
	{
		curObjectName = objName[obj];
		tool.SetTarget(position[selected]);
	}
	else
		tool.SetTarget({});
}