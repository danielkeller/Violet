#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include "UI/Elements.hpp"

#include <deque>

class Scripting;
class Persist;

class Console
{
public:
    Console(Scripting&, Persist&);
    void Draw();
    
private:
    void RunStr();
    
    Scripting& script;
    Persist& persist;
    
    bool on;
    static const int HEIGHT = 200;
    UI::SlideInOut anim;
    
    std::string current;
    std::string latest;
    UI::LineEdit edit;
    
    static const int HISTORY = 100;
    std::deque<std::string> lines;
    ptrdiff_t browse;
    ptrdiff_t browseMin, browseMax;
};

MAKE_PERSIST_TRAITS(Console, std::int64_t, std::string)

#endif
