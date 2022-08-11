// Debug needs to be set to x86

//create a roguelike game
//using Ncurses library and the doryen TCOD libtcod library
//C++
//using the C++ standard library

//the game will be a simple text based game in ACII
//the game will have a player controlled by the user
//the player will be able to move around the map
//the player will be able to interact with the map
//the map will have walls
//the map will have a player spawn point

#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <string>
#include <vector>


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
    #define NPC_PAIR       4
    #define PLAYER_PAIR    5
	#define WALL_PAIR      6
	//define a color for the light wall
	#define LIGHT_WALL_PAIR 7

	
    //====
    //use the init pair function to make a color using ncurses
    //the first parameter is the color pair number
    //the second parameter is the foreground color
    //the third parameter is the background color
    //the init_pair function must be called before any other ncurses function
    init_pair(1, COLOR_YELLOW, COLOR_GREEN);
    init_pair(2, COLOR_CYAN, COLOR_BLUE);
    init_pair(3, COLOR_BLACK, COLOR_WHITE);
    init_pair(4, COLOR_RED, COLOR_MAGENTA);
    init_pair(5, COLOR_RED, COLOR_YELLOW);
	//create a new pair with the color pair number 6 and the foreground blue and background color red
	init_pair(6, COLOR_BLUE, COLOR_RED);
	//create a new pair with the color pair number 7 and the foreground red and background color blue
	init_pair(7, COLOR_RED, COLOR_BLUE);
	
	
	
 
    //====
	//create a game loop
	//the game loop will run until the user quits the game
	//the game loop will update the game state
	//the game loop will draw the game state to the screen
	//the game loop will handle user input and update the game state
	//the game loop will wait for the next frame
    while (true)
    {
        //update the map
        engine.update();
		//render the map
        engine.render();
        //refresh the window
        refresh();//TCODConsole::flush();
    }
    return 0;
}