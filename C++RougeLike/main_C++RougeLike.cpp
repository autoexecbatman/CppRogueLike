// Debug needs to be set to x86

//we are making a roguelike game
//using the curses library
//and the classes from doryen TCOD libtcod library without using the console
#include <iostream>
#include <curses.h>

#include "main.h"
#include "Colors.h"
#include "Window.h"


//====
//an instance of the Engine class
Engine engine(30, 120);

void wizardeye()
{
    for (const auto& actor : engine.actors)
    {
        //print the actor's name
        mvprintw(actor->y, actor->x, actor->name);
    }
}

//====
//this is the main function where the game will start
int main()
{
	//====
	//initialize the color pairs
    Colors colors;
    colors.my_init_pair();

    Window window;
	
    //====
	//main game loop
    while (true)
    {
        engine.update(); // update the map
        engine.render(); // render the map
        refresh(); // refresh the window
        window.border();
        window.text();
        window.windowrefresh();
    }

    return 0;
}