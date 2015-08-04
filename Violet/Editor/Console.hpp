#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include "UI/Elements.hpp"

#include <deque>

class Scripting;

class Console
{
public:
    Console(Scripting&);
    void Draw();
    
private:
    void RunStr();
    
    Scripting& script;
    
    bool on;
    static const int HEIGHT = 200;
    UI::SlideInOut anim;
    
    std::string current;
    UI::LineEdit edit;
    
    static const int HISTORY = 100;
    std::deque<std::string> lines;
    std::deque<std::string> commands;
    size_t browseCur;
};

#endif
