#pragma once
#include <curses.h>

class Gui
{
public:
    Gui();
    ~Gui();
    void render();

protected:
    WINDOW* con; // the gui window

    void renderBar(
        int x,
        int y,
        int width,
        const char* name,
        float value, 
        float maxValue, 
        int barColor, 
        int backColor
    );
};