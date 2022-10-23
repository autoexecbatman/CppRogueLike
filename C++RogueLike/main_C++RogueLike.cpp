// Debug needs to be set to x86

// we are making a roguelike game
// using the curses library
// and the classes from doryen TCOD libtcod library without using the TCOD console

#include <iostream>
#include <curses.h>

#include "Persistent.h"
#include "Destructible.h"
#include "Attacker.h"
#include "Ai.h"
#include "Pickable.h"
#include "Container.h"
#include "Actor.h"
#include "Map.h"
#include "Engine.h"
#include "Colors.h"
#include "Window.h"
#include "Literals.h"

//==ENGINE==
// an instance of the Engine class.(singleton)
Engine engine(120_x, 30_y);

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

	while (engine.run == true && !engine.player->destructible->is_dead())
	{
		
		//==DEBUG==
		std::clog << "Running..." << countLoop << std::endl;

		//==ENGINE_FUNCTIONS==
		// TODO : Clear and redraw, then present, then wait for input.  In that order.
		// example : while (true) { clear(); refresh(); getch(); }
		
		clear(); // clear the screen

		engine.update(); // update map and actors positions and other stuff

		engine.render(); // render map and actors to the screen
		
		// TODO : move to the appropriate location.
		refresh(); // refresh the window

		// TODO : decide if you are going to use this class.
		/*window.create_window(3, 22, 60, engine.player->name);*/ // creates a resizable window

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