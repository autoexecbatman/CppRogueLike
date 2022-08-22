// Debug needs to be set to x86

//we are making a roguelike game
//using the curses library
//and the classes from doryen TCOD libtcod library without using the TCOD console
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
    while (!engine.player->destructible->isDead())
    {
        engine.update(); // update the map
        engine.render(); // render the map
        refresh(); // refresh the window
        //window.border();
        //window.text("window.wintext");
        //window.windowrefresh();

		window.create_window(3, 0, 0, engine.player->name);

		////display the players hp
		//mvprintw(engine.player->y, engine.player->x, "HP: %d", engine.player->destructible->hp);
		////display the players name
		//mvprintw(engine.player->y + 1, engine.player->x, engine.player->name);
		////display the players position
		//mvprintw(engine.player->y + 2, engine.player->x, "X: %d Y: %d", engine.player->x, engine.player->y);
		////display the players symbol
		//mvprintw(engine.player->y + 3, engine.player->x, "%c", engine.player->ch);
		////display the players color
		//mvprintw(engine.player->y + 4, engine.player->x, "Color: %d", engine.player->col);
		////display the players blocks
		//mvprintw(engine.player->y + 5, engine.player->x, "Blocks: %d", engine.player->blocks);
    }

    return 0;
}