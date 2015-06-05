#include "stdafx.h"
#include "Edit.hpp"
#include "Window.hpp"
#include "Rendering/Render.hpp"
#include "File/Persist.hpp"

#include "UI/PixelDraw.hpp"
#include "UI/Text.hpp"
#include "UI/Layout.hpp"

Edit::Edit(Render& r, RenderPasses& rp, Position& position,
	ObjectName& objName, NarrowPhase& narrowPhase, RigidBody& rigidBody,
	ComponentManager& mgr, Persist& persist)
	: enabled(true)
	, rp(rp), r(r), position(position), objName(objName), persist(persist), mgr(mgr)
	, tool(r, position)
	, focused(Object::none), selected(Object::none)
	, viewPitch(0), viewYaw(0)

	, objectNameEdit(LB_WIDTH)
	, posEdit(position), renderEdit(r, persist), collEdit(narrowPhase, r), rbEdit(rigidBody)

	, newObject("+", MOD_WIDTH), delObject("-", MOD_WIDTH)
	, objectSelect(LB_WIDTH)
{
}

void Edit::Editable(Object o)
{
	auto& name = objName[o];
	auto it = std::lower_bound(objectSelect.items.begin(),
		objectSelect.items.end(), name,
		[](std::pair<Object, std::string>& l, const std::string& r)
	{return l.second < r; });

	objectSelect.items.try_emplace(it, o, name);
}

void Edit::Load(const Persist& persist)
{
	for (auto o : persist.GetAll<Edit>())
		Editable(std::get<0>(o));
}

void Edit::Unload(const Persist& persist)
{
	for (auto o : persist.GetAll<Edit>())
		Remove(std::get<0>(o));
}

bool Edit::Has(Object obj) const
{
	return objectSelect.items.count(obj) != 0;
}

void Edit::Save(Object obj, Persist& persist) const
{
	if (objectSelect.items.count(obj))
		persist.Set<Edit>(obj);
	else
		persist.Delete<Edit>(obj);
}

void Edit::Remove(Object obj)
{
	objectSelect.items.erase(obj);
}

void Edit::PhysTick(Object camera)
{
	Events& e = UI::FrameEvents();

	if (!enabled)
	{
		if (e.PopKeyEvent({ { GLFW_KEY_ESCAPE, 0 }, GLFW_PRESS }))
			enabled = true;
		return;
	}

	Object picked = rp.Pick(e.MousePosPxl());
	Object newSelect = selected;

	if (e.MouseClick(GLFW_MOUSE_BUTTON_LEFT))
    {
		if (picked == Object::none) //click outside to deselect
			newSelect = Object::none;
		else if (objectSelect.items.count(picked)) //click to select
			newSelect = picked;

		//don't register picks outside of the viewport
		if (picked != Object::invalid)
			focused = picked;
	}
	//save the object once we stop moving
	else if (e.MouseRelease(GLFW_MOUSE_BUTTON_LEFT)
		&& selected != Object::none
		&& selected != focused) //a hack to see if we were dragging the tool
		position.Save(selected, persist);

	//focus reverts to the selectable object that was selected
	if (!e.MouseButton(GLFW_MOUSE_BUTTON_LEFT))
		focused = selected;

	if (selected != Object::none && position.Has(selected))
		tool.SetTarget(position[selected]);
	else
		tool.SetTarget({});

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
	
	UI::LayoutStack& l = UI::CurLayout() = UI::LayoutStack(e.dimVec, UI::Layout::Dir::Right);

	enabled = !slide.Draw(LB_WIDTH);

	renderEdit.DrawPicker(selected, persist);

	//left bar
	l.PushNext(UI::Layout::Dir::Down);
	l.EnsureWidth(LB_WIDTH);

	if (selected != Object::none)
	{
		if (objectNameEdit.Draw(curObjectName))
		{
			objName.Rename(selected, curObjectName);
			//so it comes back alphabetically
			Remove(selected); Editable(selected);
		}

		l.PutSpace(UI::LINEH);
		posEdit.Draw(persist, selected);
		renderEdit.Draw(persist, selected);
		collEdit.Draw(persist, selected);
		rbEdit.Draw(persist, selected);
		l.PutSpace(UI::LINEH);
	}

	l.PushNext(UI::Layout::Dir::Right);

	if (newObject.Draw())
	{
		Object n;
		Editable(n);
		Save(n, persist);
	}

	UI::DrawText("objects", l.PutSpace(LB_WIDTH - 2 * MOD_WIDTH));
	
	if (selected != Object::none && delObject.Draw())
	{
		mgr.Delete(selected);
		mgr.Save(selected, persist);
		newSelect = Object::none;
	}

	l.Pop();

	objectSelect.Draw(newSelect);

	UI::DrawBox(l.Current());
	UI::DrawShadow(l.Current());

	l.Pop();

	//actually change selection at the end so objects have a chance to save when it changes
	if (selected != newSelect)
	{
		selected = newSelect;

		renderEdit.ClosePicker();

		if (selected != Object::none)
			curObjectName = objName[selected];
	}

	if (e.PopKeyEvent({ { GLFW_KEY_ESCAPE, 0 }, GLFW_PRESS }))
		slide.Close();

	//fixme
	e.PopMouse();
	e.PopScroll();
}

template<>
const char* PersistSchema<Edit>::name = "edit";
template<>
Columns PersistSchema<Edit>::cols = { "object" };