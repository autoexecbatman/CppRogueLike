// Debug needs to be set to x86

//we are making a roguelike game
//using the curses library
//and the classes from doryen TCOD libtcod library without using the console

#include <curses.h>//PDCurses is the curses library

#include "main.h"

//make an instance of the Engine class
Engine engine(30, 120);

//this is the main function where the game will start
int main()
{
//====
//define the color pairs for the game
#define GRASS_PAIR     1
#define EMPTY_PAIR     1
#define WATER_PAIR     2
#define MOUNTAIN_PAIR  3
#define ORC_PAIR       4
#define PLAYER_PAIR    5
#define WALL_PAIR      6
#define LIGHT_WALL_PAIR 7
#define LIGHT_GROUND_PAIR 8
#define DEAD_NPC_PAIR 9
#define TROLL_PAIR 10
	
    //====
    //use the init pair function to make a color using curses
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
	
    //====

	//main game loop
    while (true)
    {
        //update the map
        engine.update();
		//render the map
        engine.render();
        //refresh the window
        refresh();
    }
    return 0;
}