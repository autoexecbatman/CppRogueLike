// Debug needs to be set to x86
//we are making a roguelike game
//using the curses library
//and the classes from doryen TCOD libtcod library without using the console

#include <curses.h>

#include "main.h"
#include "Colors.h"


//====
//an instance of the Engine class
Engine engine(30, 120);

//====
//this is the main function where the game will start
int main()
{
	//====
	//initialize the color pairs
    Colors colors;
    colors.my_init_pair();

    //====
	//main game loop
    while (true)
    {
        engine.update(); // update the map
        engine.render(); // render the map
        refresh(); // refresh the window 
    }

    return 0;
}