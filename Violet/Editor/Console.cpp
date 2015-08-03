#include "stdafx.h"
#include "Console.hpp"

#include "Window.hpp"
#include "UI/PixelDraw.hpp"
#include "UI/Text.hpp"
#include "UI/Layout.hpp"

Console::Console()
    : on(false)
{}

void Console::Draw()
{
    if (!on)
    {
        if (UI::FrameEvents().PopKey({ GLFW_KEY_ESCAPE, 0 }))
        {
            edit.focus.StealFocus();
            on = true;
        }
        return;
    }
    
    UI::LayoutStack& l = UI::CurLayout();
    l.PushLayer(UI::Layout::Dir::Up);
    int width = l.Current().Box().sizes().x();
    UI::PushZ(8);
    
    if (UI::FrameEvents().PopKey({ GLFW_KEY_ESCAPE, 0 }))
    {
        edit.focus.Unfocus();
        anim.Close();
    }
    if (anim.Draw(HEIGHT))
        on = false;
    
    //before edit takes the event
    if (edit.focus.focused && UI::FrameEvents().PopKey({ GLFW_KEY_ENTER, 0 }))
        current.clear(); //run it
    
    //a little bottom padding
    l.PutSpace(UI::TEXT_PADDING);
    
    edit.width = width;
    edit.Draw(current);
    
    l.PutSpace(HEIGHT);
    
    UI::AlignedBox2i box = l.Current().Filled();
    UI::DrawBox(box);
    UI::DrawShadow(box);
    
    UI::PopZ();
    l.PopLayer();
}
