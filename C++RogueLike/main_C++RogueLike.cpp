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
#include <ranges>

// https://pdcurses.org/docs/MANUAL.html
#include <curses.h>

#include "Game.h"
#include "Colors/Colors.h"
#include "Menu/Menu.h"
#include "Menu/MenuGender.h"
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

	//==PLAYER==
	game.create_player(); // player is initialized here to get data from menu

	//==INIT_MENU==
	game.menus.push_back(std::make_unique<Menu>());

	Gui gui;
	int loopNum{ 0 };
	while (game.run) // main game loop
	{
		if (!game.menus.empty())
		{
			game.windowState = Game::WindowState::MENU;
		}
		else
		{
			game.windowState = Game::WindowState::GAME;
		}

		//==MENU==
		switch (game.windowState)
		{
		case Game::WindowState::MENU:
		{
			if (!game.menus.empty())
			{
				game.menus.back()->menu();
				// if back is pressed, pop the menu
				if (game.menus.back()->back)
				{
					game.menus.pop_back();
					game.menus.back()->menu_set_run_true();
				}

				if (!game.menus.back()->run)
				{
					game.menus.pop_back();
				}

				game.shouldInput = false;
			}
		}
		break;

		case Game::WindowState::GAME:
		{
			if (!game.gameInit)
			{
				game.init();
				game.gameInit = true;
			}
			//==DEBUG==
			game.log("//====================LOOP====================//");
			game.log("Loop number: " + std::to_string(loopNum) + "\n");

			//==INIT_GUI==
			if (!gui.guiInit)
			{
				gui.gui_init();
				gui.guiInit = true;
			}

			//==INPUT==
			game.keyPress = ERR; // reset keyPress
			if (game.shouldInput)
			{
				game.key_store();
				game.key_listen();
			}
			game.shouldInput = true; // reset shouldInput

			//==UPDATE==
			game.log("Running update...");
			game.update(); // update map and actors positions
			gui.gui_update(); // update the gui
			game.log("Update OK.");
			// **NEW**: If a menu was added, skip the rest of this loop
			if (!game.menus.empty())
			{
				game.windowState = Game::WindowState::MENU;
				continue;
			}

			//==DRAW==
			game.log("Running render...");
			clear();
			game.render(); // render map and actors to the screen
			gui.gui_render(); // render the gui
			refresh();
			game.log("Render OK.");
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
