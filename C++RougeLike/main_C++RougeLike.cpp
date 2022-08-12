// Debug needs to be set to x86

//we are making a roguelike game
//using the curses library
//and the classes from doryen TCOD libtcod library
//using the C++ standard library
//and the C standard library

//the game will be a simple text based game in ACII
//the game will have a player controlled by the user
//the player will be able to move around the map
//the player will be able to interact with the map
//the map will have walls
//the map will have a player spawn point
//the game will run only in the curses window using curses


#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <string>


#include "libtcod.hpp"
#include "Actor.h"
#include "Map.h"
#include "Engine.h"

//make an instance of the Engine class
Engine engine;

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
	//define a color for the light wall
	#define LIGHT_WALL_PAIR 7
	//define a color for the light ground
	#define LIGHT_GROUND_PAIR 8
    #define TROLL_PAIR 10
	
    //====
    //use the init pair function to make a color using ncurses
    //the first parameter is the color pair number
    //the second parameter is the foreground color
    //the third parameter is the background color
    //the init_pair function must be called before any other ncurses function
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLUE);
    init_pair(3, COLOR_BLACK, COLOR_WHITE);
    init_pair(4, COLOR_RED, COLOR_BLACK);
	//a pair color for the player
    init_pair(5, COLOR_WHITE, COLOR_BLACK);
	//create a new pair with the color pair number 6 and the foreground blue and background color red
	init_pair(6, COLOR_BLUE, COLOR_RED);
	//create a new pair with the color pair number 7 and the foreground red and background color blue
	init_pair(7, COLOR_RED, COLOR_WHITE);
	//create a new pair with the color pair number 8 and the foreground green and background color yellow
	init_pair(8, COLOR_GREEN, COLOR_YELLOW);
    init_pair(10, COLOR_GREEN, COLOR_BLACK);
	
	
	
 
    //====
	//main loop
	//the main loop will run until the user quits the game by pressing the q key
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