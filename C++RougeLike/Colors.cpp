#include <curses.h>
#include "Colors.h"



void Colors::my_init_pair()
{
    //====
    //the init_pair functions to make a color using curses
    //the first parameter is the color pair number
    //the second parameter is the foreground color
    //the third parameter is the background color
    //the init_pair functions must be called before any other curses functions
    init_pair(1, COLOR_WHITE, COLOR_BLACK);//grass color
    init_pair(2, COLOR_CYAN, COLOR_BLUE);//water color
    init_pair(3, COLOR_BLACK, COLOR_WHITE);//mountain color
    init_pair(4, COLOR_RED, COLOR_BLACK);//orc color
    init_pair(5, COLOR_WHITE, COLOR_BLACK);//player color
    init_pair(6, COLOR_BLUE, COLOR_RED);//wall color
    init_pair(7, COLOR_RED, COLOR_WHITE);//light wall color
    init_pair(8, COLOR_GREEN, COLOR_YELLOW);//light ground color
    init_pair(9, COLOR_RED, COLOR_BLACK);//dead npc color
    init_pair(10, COLOR_GREEN, COLOR_BLACK);//troll color
}