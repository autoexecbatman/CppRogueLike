// Debug needs to be set to x86

//we are making a roguelike game
//using the curses library
//and the classes from doryen TCOD libtcod library without using the TCOD console

#include <iostream>
#include <curses.h>

#include "main.h" // TODO : find a better way to include multiple headers.
#include "Colors.h"
#include "Window.h"

//==ENGINE==
// an instance of the Engine class.
Engine engine(30, 120); // TODO : check what is going on with the width/height of the console window.

// TODO : find an appropriate location for this function.
void wizardeye()
{
	for (const auto& actor : engine.actors)
	{
		// print the actor's name
		mvprintw(actor->posY, actor->posX, actor->name);
	}
}

//==MAIN_FUNCTION==
// This is the main function where the game will run.
int main()
{
	//==COLORS==
	//initialize the color pairs
	Colors colors;
	colors.my_init_pair();

	//==WINDOW==
	// create a window object
	Window window;
	
	//==ENGINE==
	// load the starting menu
	engine.game_menu();

	//==GAME_LOOP==
	// main game loop
	int countLoop = 0;

	while (engine.run == true && !engine.player->destructible->isDead())
	{
		
		//==DEBUG==
		std::clog << "Running..." << countLoop << std::endl;

		//==ENGINE_FUNCTIONS==
		// TODO : Clear and redraw, then present, then wait for input.  In that order.
		// example : while (true) { clear(); refresh(); getch(); }

		engine.update(); // update map and actors positions and other stuff

		clear();

		engine.render(); // render map and actors to the screen
		
		// TODO : move to the appropriate location.
		refresh(); // refresh the window

		// TODO : decide if you are going to use this class.
		window.create_window(3, 22, 60, engine.player->name); // create a resizable window

		//==INPUT==
		// get the input from the player
		engine.lastKey = engine.keyPress;
		engine.keyPress = getch();

		countLoop++;
	}
	//==ENGINE==
	// save the game after the loop breaks
	engine.save();

	return 0;
}