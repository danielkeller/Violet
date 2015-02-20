#include "stdafx.h"
#include "Edit.hpp"
#include "Window.hpp"
#include "Rendering/Render.hpp"

Edit::Edit(Render& r, Window& w, Position& position)
	: r(r), w(w), position(position), pick(r, w), tool(r, position)
    , focused(Object::none), mouseDown(false)
	, viewPitch(0), viewYaw(0)
{
    r.PassDefaults(PickerPass, pick.shader, {});
}

void Edit::Editable(Object o)
{
    editable.insert(o);
}

void Edit::PhysTick(Object camera)
{
    tool.Update(w, camera, focused);
	if (w.LeftMouse() && !mouseDown) //just clicked
    {
        Object picked = pick.Picked();
		if (selected == picked) //click to deselect
		{
			if (selected != Object::none)
				tool.SetTarget({});
			selected = Object::none;
		}
		else if (editable.count(picked)) //click to select
		{
			selected = picked;
			if (selected != Object::none)
				tool.SetTarget(position[selected]);
		}
		focused = picked;
        mouseDown = true;
    }
	else if (!w.LeftMouse() && mouseDown) //just released
    {
		//focus reverts to the selectable object that was selected
		focused = selected;
        mouseDown = false;
	}

	pick.Highlight(pick.Picked(), Picker::Hovered);
	pick.Highlight(focused, Picker::Focused);
	pick.Highlight(selected, Picker::Selected);
    
	//right mouse to rotate
    if (w.RightMouse())
    {
		viewPitch -= w.MouseDeltaScr().x();
		viewYaw += w.MouseDeltaScr().y();
		position[camera]->rot = Eigen::AngleAxisf(viewYaw, Vector3f::UnitX())
						      * Eigen::AngleAxisf(viewPitch, Vector3f::UnitZ());
    }

	position[camera]->pos *= (1 - w.ScrollDelta().y()*.05f);
}

void Edit::DrawTick()
{
    pick.Pick();
}
