#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include "UI/Elements.hpp"

class Console
{
public:
    Console();
    void Draw();
    
private:
    bool on;
    static const int HEIGHT = 200;
    UI::SlideInOut anim;
    
    std::string current;
    UI::LineEdit edit;
};

#endif
