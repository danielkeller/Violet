#include "stdafx.h"
#include "Console.hpp"

#include "Window.hpp"
#include "UI/PixelDraw.hpp"
#include "UI/Text.hpp"
#include "UI/Layout.hpp"

#include "Script/Scripting.hpp"
#include "File/Persist.hpp"

Console::Console(Scripting& script, Persist& persist)
    : script(script), persist(persist), on(false)
{
    //run dummy query to create database
    persist.Exists<Console>(0);
    
    std::tie(browseMin, browseMax) = persist.Database()
        .MakeStmt("select min(id), max(id)+1 from console_history")
        .Eval<std::int64_t, std::int64_t>();
    browse = browseMax;
}

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
        
        if (UI::FrameEvents().PopKey({ GLFW_KEY_UP, 0 }) && browse > browseMin)
        {
            //save off the edited line so it can be browsed back to
            if (browse == browseMax)
                latest = current;
            --browse;
            std::tie(std::ignore, current) = persist.Get<Console>(browse);
        }
        if (UI::FrameEvents().PopKey({ GLFW_KEY_DOWN, 0 }) && browse < browseMax)
        {
            ++browse;
            if (browse == browseMax)
                current = latest;
            else
                std::tie(std::ignore, current) = persist.Get<Console>(browse);
        }
    }
    
    edit.width = width;
    std::string old = current;
    edit.Draw(current);
    
    //string was edited
    if (current != old)
        browse = browseMax;
    
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
        
        //don't put in repeated or empty commands
        if (browse != browseMax - 1 && current != "")
            persist.Set<Console>(browseMax++, current);
        
        browse = browseMax;
        lines.emplace_front(std::move(current));
        
        std::string line;
        while (std::getline(result, line, '\n'))
            lines.emplace_front(std::move(line));
        
        if (lines.size() > HISTORY)
            lines.pop_back();
        if (browseMax - browseMin > HISTORY)
            persist.Delete<Console>(browseMin++);
        
        current.clear();
    } catch (std::runtime_error& err) {
        lines.emplace_front(err.what());
    }
}

template<>
const char* PersistSchema<Console>::name = "console_history";
template<>
Columns PersistSchema<Console>::cols = { "id", "command" };
