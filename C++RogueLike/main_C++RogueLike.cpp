//==Debug-MemoryLeaks==
// https://learn.microsoft.com/en-us/cpp/c-runtime-library/find-memory-leaks-using-the-crt-library?view=msvc-170
#define _CRTDBG_MAP_ALLOC // enable debug memory allocation
#include <stdlib.h> // define malloc, free
#include <crtdbg.h> // define _CrtDumpMemoryLeaks
//====
// file: main_C++RogueLike.cpp
// we are making a rogue-like game in C++ using the PDCurses library and the libtcod library
#include <iostream>
#include <fstream>
#include <string>

// https://pdcurses.org/docs/MANUAL.html
#include <curses.h> 

#include "Colors.h"
#include "Game.h"
#include "Menu.h"
#include "Gui.h"

#include "StrengthAttributes.h"

#include "ChatGPT.h" // for openai::start
#include "include/user_config.h"

Game game;

int main()
{

	//==DEBUG_MEMORY_LEAKS==
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	/*_CrtSetBreakAlloc(174);*/

	//==OPENAI_API==
	openai::start(API_KEY, "");

	//==DEBUG_STREAM==
	std::ofstream debugFile("clog.txt"); // create a file to store debug info
	std::clog.rdbuf(debugFile.rdbuf()); // redirect std::clog to the file
	
	//==INIT_CURSES==
	std::clog << "Initializing curses...\n";
	initscr(); // initialize the screen in curses
	std::clog << "Initialized successfully.\n";

	if (!has_colors()) {
		std::clog << "Colors not supported.\n";
		std::cout << "Colors not supported.\n";
		throw std::runtime_error("Colors not supported");
	}

	std::clog << "Initializing colors...\n";
	start_color(); // start color curses
	std::clog << "Initialized successfully.\n";
	std::clog << "Initializing color pairs...\n";
	Colors colors;
	colors.my_init_pair();
	std::clog << "Initialized successfully.\n";

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

	printw("Welcome to C++RogueLike!"); // print a welcome message
	printw("Console size: %d x %d", COLS, LINES); // print the console size
	refresh(); // refresh the screen

	//==PLAYER==
	game.create_player(); // the player is initialized here because it needs to get data from menu selections

	//==INIT_MENU==
	Menu menu;
	menu.menu();
	clear(); // finished with the menu, clear the screen

	refresh(); // starting new drawing, refresh the screen
	//==INIT_GUI==
	Gui gui;
	gui.gui_init();

	int countLoop{ 0 };
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
		std::clog << "initialized successfully." << std::endl;

		//==DRAW==
		std::clog << "initializing game render..." << std::endl;
		/*clear();*/
		game.render(); // render map and actors to the screen
		gui.gui_render(); // render the gui
		std::clog << "initialized successfully." << std::endl;

		//==INPUT==
		game.key_store();
		game.key_listen();

		countLoop++;
	}
	
	game.save_all();
	gui.gui_shutdown();
	endwin();
	if (isendwin())
	{
		std::clog << "Curses shutdown successfully.\n";
		std::cout << "Curses shutdown successfully.\n";
	}
	else
	{
		std::clog << "Curses shutdown failed.\n";
		std::cout << "Curses shutdown failed.\n";
	}

	print_chart();

	debugFile.close();

	/*_CrtDumpMemoryLeaks();*/
	return 0;
}

// end of file: main_C++RogueLike.cpp
