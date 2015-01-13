#include "stdafx.h"
#include "Edit.hpp"
#include "Window.hpp"

Edit::Edit(Render& r, Window& w, Mobile& m)
    : w(w), m(m), pick(r, w), tool(r, m)
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
    if (w.LeftMouse())
    {
        if (!mouseDown) //just clicked
        {
            Object picked = pick.Picked();
            if (editable.count(picked))
                pick.Highlight(selected = picked);
            //focused = 
        }
        mouseDown = true;
    }
    else
    {
        mouseDown = false;
        focused = Object::none;

        //hover highlight
        pick.Highlight(pick.Picked());
    }
    
    if (w.LeftMouse() && focused == Object::none)
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
