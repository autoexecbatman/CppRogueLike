// Debug needs to be set to x86
//we are making a roguelike game
//using the curses library
//and the classes from doryen TCOD libtcod library without using the console
#include <iostream>
#include <curses.h>

#include "main.h"
#include "Colors.h"


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

    //====
    //add a new curses window for printing debug information
    // newwin() creates a new window with the given number of lines,
    //nlines and columns, ncols.
    //The upper left corner of the window is at line begy, column begx.
    //If nlines is zero, it defaults to LINES - begy; ncols to COLS - begx.
    //Create a new full-screen window by calling newwin(0, 0, 0, 0).
    //WINDOW* newwin(int nlines, int ncols, int begy, int begx);
	
    //====
	//main game loop
    while (true)
    {
        engine.update(); // update the map
        engine.render(); // render the map
        refresh(); // refresh the window
		
		// need to refresh stdscr before window
        WINDOW* win = newwin(3, 20, 0, 0);
        wborder(
            win,
            '-', 
            '-', 
            '-', 
            '-', 
            '-', 
            '-',
			'-',
			'-'
        );
        mvwprintw(win, 1, 1, "something");
        wrefresh(win);
    }

    return 0;
}