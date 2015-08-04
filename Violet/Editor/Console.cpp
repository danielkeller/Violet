#include "stdafx.h"
#include "Console.hpp"

#include "Window.hpp"
#include "UI/PixelDraw.hpp"
#include "UI/Text.hpp"
#include "UI/Layout.hpp"

#include "Script/Scripting.hpp"

Console::Console(Scripting& script)
    : script(script), on(false)
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
        RunStr();
    
    //a little bottom padding
    l.PutSpace(UI::TEXT_PADDING);
    
    edit.width = width;
    edit.Draw(current);
    
    auto it = lines.cbegin();
    for (int h = 0; h < HEIGHT; h += UI::LINEH)
    {
        if (it != lines.cend())
            UI::DrawText(*it++, l.PutSpace(UI::LINEH), UI::TextAlign::Left);
        else
            l.PutSpace(UI::LINEH);
    }

    UI::AlignedBox2i box = l.Current().Filled();
    UI::DrawBox(box);
    UI::DrawShadow(box);
    
    UI::PopZ();
    l.PopLayer();
}

void Console::RunStr()
{
    try {
        std::stringstream result{ script.RunStr(current) };
        
        lines.emplace_front(std::move(current));
        
        std::string line;
        while (std::getline(result, line, '\n'))
            lines.emplace_front(std::move(line));
        
        if (lines.size() > HISTORY)
            lines.pop_back();
        
        current.clear();
    } catch (std::runtime_error& err) {
        lines.emplace_front(err.what());
    }
}
