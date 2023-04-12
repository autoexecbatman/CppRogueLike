// Debug needs to be set to x86
// we are making a rogue-like game in C++ using the PDCurses library and the libtcod library
#include <iostream>
#include <fstream>
#include <string>

#include <curses.h>

#include "Colors.h"
#include "Game.h"
#include "Menu.h"
#include "Gui.h"
#include "Player.h"

Game game;

// TODO : extract the player from the game class

int main()
{
	//==DEBUG_STREAM==
	std::ofstream debugFile("clog.txt"); // create a file to store debug info
	std::clog.rdbuf(debugFile.rdbuf()); // redirect std::clog to the file

	//==INIT_CURSES==
	std::clog << "Initializing curses...\n";
	initscr(); // initialize the screen in curses
	std::clog << "Initialized successfully.\n";

	if (has_colors()) // check if the terminal supports colors
	{
		std::clog << "Initializing colors...\n";
		start_color(); // start color curses
		std::clog << "Initialized successfully.\n";
		std::clog << "Initializing color pairs...\n";
		Colors colors;
		colors.my_init_pair();
		std::clog << "Initialized successfully.\n";
	}
	else
	{
		std::clog << "Colors not supported.\n";
		std::cout << "Colors not supported.\n";
		exit(-1);
	}

	cbreak(); // disable line buffering
	noecho(); // turn off echoing of keys to the screen
	curs_set(0); // remove the blinking cursor
	keypad(stdscr, true); // enable the keypad for non-char keys

	if (has_mouse())
	{
		std::clog << "Initializing mouse...\n";
		mousemask(0x1fffffffL, nullptr); // enable mouse events * #define ALL_MOUSE_EVENTS        0x1fffffffL
		std::clog << "Initialized successfully.\n";
	}
	else
	{
		std::clog << "Mouse not supported.\n";
		std::cout << "Mouse not supported.\n";
		exit(-1);
	}

	refresh(); // refresh the screen

	//==INIT_MENU==
	Menu menu;
	menu.menu();
	clear(); // finished with the menu, clear the screen

	refresh(); // startnig new drawing, refresh the screen
	//==INIT_GUI==
	Gui gui;
	gui.gui_init();

	//==INIT_PLAYER==
	// extract the player from the game class
	Player player( 40, 25 );

	auto countLoop{ 0 }; // count the number of loops
	while (game.run == true)
	{
		//==DEBUG==
		std::clog << "//====================LOOP====================//\n";
		std::string debug{ "Loop number: " + std::to_string(countLoop) + "\n" };
		std::clog << debug << std::endl;

		//==UPDATE==
		std::clog << "initializing game update..." << std::endl;
		game.update(); // update map and actors positions
		gui.gui_update(); // update the gui
		player.update(); // update the player
		std::clog << "initialized successfully." << std::endl;
		
		//==DRAW==
		std::clog << "initializing game render..." << std::endl;
		/*clear();*/
		game.render(); // render map and actors to the screen
		gui.gui_render(); // render the gui
		player.render(); // render the player
		std::clog << "initialized successfully." << std::endl;

		// print the player's gender
		mvprintw(
			1,
			1,
			"Gender: %s",
			game.player->gender.c_str()
		);

		// print the player's class
		mvprintw(
			2,
			1,
			"Class: %s",
			game.player->playerClass.c_str()
		);

		// print the player's name
		mvprintw(
			3,
			1,
			"Name: %s",
			game.player->name.c_str()
		);

		//==INPUT==
		// get the input from the player
		std::clog << "storing key" << std::endl;
		game.lastKey = game.keyPress;

		std::clog << "getting key" << std::endl;
		game.keyPress = getch();

		countLoop++;
	}
	std::clog << "Closing the game..." << std::endl;
	game.save(); // save the game
	
	endwin(); // close the curses window

	debugFile.close(); // close the stream to serialize the std::clog output

	return 0;
}