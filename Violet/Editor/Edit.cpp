#include "stdafx.h"
#include "Edit.hpp"
#include "Window.hpp"
#include "Rendering/Render.hpp"
#include "Persist.hpp"

Edit::Edit(Render& r, RenderPasses& rp, Window& w, Position& position)
	: w(w), rp(rp), position(position)
	, tool(r, position), focused(Object::none)
	, viewPitch(0), viewYaw(0)
{
}

void Edit::Editable(Object o)
{
    editable.insert(o);
}

void Edit::PhysTick(Object camera)
{
	Object picked = rp.Pick(w.MousePosView());

	if (w.LeftMouseClick())
    {
		if (picked == Object::none) //click outside to deselect
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
    }
	else if (w.LeftMouseRelease()) //just released
    {
		//save the object once we stop moving
		if (selected != Object::none)
			position.Save(selected);

		//focus reverts to the selectable object that was selected
		focused = selected;
	}

	tool.Update(w, camera, focused);

	rp.Highlight(picked, RenderPasses::Hovered);
	rp.Highlight(focused, RenderPasses::Focused);
	rp.Highlight(selected, RenderPasses::Selected);
    
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
