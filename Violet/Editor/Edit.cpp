#include "stdafx.h"
#include "Edit.hpp"
#include "Window.hpp"

Edit::Edit(Render& r, Window& w, Mobile& m)
    : r(r), w(w), m(m), pick(r, w), tool(r, m)
    , focused(Object::none), mouseDown(false)
{
    r.PassDefaults(PickerPass, pick.shader, {});
}

void Edit::Editable(Object o)
{
    editable.insert(o);
}

void Edit::PhysTick()
{
    tool.Update(w, focused);
	if (w.LeftMouse() && !mouseDown) //just clicked
    {
        Object picked = pick.Picked();
		if (selected == picked) //click to deselect
		{
			if (selected != Object::none)
				tool.Move().Remove(r.GetLocProxyFor(selected));
			selected = Object::none;
		}
		else if (editable.count(picked)) //click to select
		{
			selected = picked;
			if (selected != Object::none)
				tool.Move().Add(r.GetLocProxyFor(selected));
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
        m.CameraLoc().rot *= Quaternionf{
            Eigen::AngleAxisf(-w.MouseDeltaScr().x(),
                              Vector3f::UnitZ()) }; //rotate around world z
        m.CameraLoc().rot *= Quaternionf{
            Eigen::AngleAxisf(w.MouseDeltaScr().y(),
                              //rotate around camera x
                              m.CameraLoc().rot.conjugate() * Vector3f::UnitX()) };
    }
}

void Edit::DrawTick()
{
    pick.Pick();
}
