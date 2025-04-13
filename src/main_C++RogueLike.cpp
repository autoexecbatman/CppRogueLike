//==Debug-MemoryLeaks==
// https://learn.microsoft.com/en-us/cpp/c-runtime-library/find-memory-leaks-using-the-crt-library?view=msvc-170
#define _CRTDBG_MAP_ALLOC // enable debug memory allocation
#include <stdlib.h> // define malloc, free
//#include <crtdbg.h> // define _CrtDumpMemoryLeaks
//====
// file: main_C++RogueLike.cpp
// we are making a rogue-like game in C++ using the PDCurses library and the libtcod library
#include <iostream>
#include <fstream>
#include <string>

// https://pdcurses.org/docs/MANUAL.html
#include <curses.h>

#include "Game.h"
#include "Colors/Colors.h"
#include "Menu/Menu.h"
#include "Gui/Gui.h"

#include "Attributes/StrengthAttributes.h" // for debugging
#include "Weapons.h" // for debugging

void init_curses(); // declaration of a function that handles curses procedures

Game game; // create a global game object

int main()
{
	//==DEBUG_MEMORY_LEAKS==
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	/*_CrtSetBreakAlloc(1779);*/

	//==DEBUG_STREAM==
	std::ofstream debugFile("clog.txt"); // create a file to store debug info
	std::clog.rdbuf(debugFile.rdbuf()); // redirect std::clog to the file

	//==INIT_CURSES==
	init_curses();

	//==INIT_MENU==
	game.menus.push_back(std::make_unique<Menu>());

	Gui gui;
	int loopNum{ 0 };
	while (game.run) // main game loop
	{
		game.menus.empty() ? game.windowState = Game::WindowState::GAME : game.windowState = Game::WindowState::MENU;

		//==MENU==
		switch (game.windowState)
		{
		case Game::WindowState::MENU:
		{
			game.handle_menus();
		}
		break;

		case Game::WindowState::GAME:
		{
			game.handle_gameloop(gui, loopNum);
		}
		break;

		default:
			break;
		}

		loopNum++;
	}

	game.save_all();

	// make sure to shut down all windows before exiting the program
	gui.gui_shutdown();
	endwin();
	if (isendwin())
	{
		game.log("Curses shutdown successfully.");
	}
	else
	{
		game.log("Curses shutdown failed.");
		exit(EXIT_FAILURE);
	}

	StrengthAttributes strength;
	strength.print_chart(); // print strength chart for debugging

	Weapons weapons;
	weapons.print_chart(); // print weapons chart for debugging

	debugFile.close();
	/*_CrtDumpMemoryLeaks();*/
	return 0;
}

// a chain of functions to initialize curses
void init_curses()
{
	//==INIT_CURSES==
	initscr(); // initialize the screen in curses

	if (!has_colors()) {
		game.log("Colors not supported.");
	}
	else
	{
		start_color(); // start color curses
		Colors colors;
		colors.my_init_pair();
	}

	cbreak(); // disable line buffering
	noecho(); // turn off echoing of keys to the screen
	curs_set(0); // remove the blinking cursor
	keypad(stdscr, true); // enable the keypad for non-char keys

	if (has_mouse())
	{
		mousemask(ALL_MOUSE_EVENTS, nullptr); // enable mouse events * #define ALL_MOUSE_EVENTS        0x1fffffffL
	}
	else
	{
		game.log("Mouse not supported.");
	}

	printw("Welcome to C++RogueLike!"); // print a welcome message
	printw("Console size: %d x %d", COLS, LINES); // print the console size
	refresh(); // refresh the screen
	game.log("refresh called"); // log the refresh call
}

// end of file: main_C++RogueLike.cpp
