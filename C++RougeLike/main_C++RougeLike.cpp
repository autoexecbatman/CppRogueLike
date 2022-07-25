// Debug needs to be set to x86
#include <curses.h>
#include "libtcod.hpp"
#include "Actor.h"
#include "Map.h"
#include "Engine.h"

Engine engine;
int main()
{
    /*Engine engine;*/
    //int player_x = engine.player->x;
    //int player_y = engine.player->y;
    //====
#define GRASS_PAIR     1
#define EMPTY_PAIR     1
#define WATER_PAIR     2
#define MOUNTAIN_PAIR  3
#define NPC_PAIR       4
#define PLAYER_PAIR    5
//====
    init_pair(1, COLOR_YELLOW, COLOR_GREEN);
    init_pair(2, COLOR_CYAN, COLOR_BLUE);
    init_pair(3, COLOR_BLACK, COLOR_WHITE);
    init_pair(4, COLOR_RED, COLOR_MAGENTA);
    init_pair(5, COLOR_RED, COLOR_YELLOW);
    //====
    while (true)
    {
        
        engine.update();
        engine.render();
        
        refresh();//TCODConsole::flush();
    }
    return 0;
}