#include "stdafx.h"
#include "Console.hpp"

#include "Window.hpp"
#include "UI/PixelDraw.hpp"
#include "UI/Text.hpp"
#include "UI/Layout.hpp"

#include "Script/Scripting.hpp"

Console::Console(Scripting& script)
    : script(script), on(false), commands({""}), browseCur()
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
    
    //a little bottom padding
    l.PutSpace(UI::TEXT_PADDING);
    
    //before edit takes the events
    if (edit.focus.focused)
    {
        if (UI::FrameEvents().PopKey({ GLFW_KEY_ENTER, 0 }))
            RunStr();
        
        if (UI::FrameEvents().PopKey({ GLFW_KEY_UP, 0 }) && browseCur < commands.size() - 1)
        {
            ++browseCur;
            current = commands[browseCur];
        }
        if (UI::FrameEvents().PopKey({ GLFW_KEY_DOWN, 0 }) && browseCur > 0)
        {
            --browseCur;
            current = commands[browseCur];
        }
    }
    
    edit.width = width;
    edit.Draw(current);
    
    //string was edited
    if (current != commands[browseCur])
    {
        browseCur = 0;
        commands[0] = current;
    }
    
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
        
        commands.emplace_front(current);
        lines.emplace_front(std::move(current));
        
        std::string line;
        while (std::getline(result, line, '\n'))
            lines.emplace_front(std::move(line));
        
        if (lines.size() > HISTORY)
            lines.pop_back();
        if (commands.size() > HISTORY)
            commands.pop_back();
        
        current.clear();
    } catch (std::runtime_error& err) {
        lines.emplace_front(err.what());
    }
}
