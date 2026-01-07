//==Debug-MemoryLeaks==
// https://learn.microsoft.com/en-us/cpp/c-runtime-library/find-memory-leaks-using-the-crt-library?view=msvc-170
//#define _CRTDBG_MAP_ALLOC // enable debug memory allocation
#include <stdlib.h> // define malloc, free
//#include <crtdbg.h> // define _CrtDumpMemoryLeaks

#ifdef EMSCRIPTEN
#define SDL_MAIN_HANDLED
#include <SDL.h>
/* External declarations for PDCurses SDL integration */
#endif

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

#ifdef EMSCRIPTEN
PDCEX SDL_Window* pdc_window;
PDCEX SDL_Surface* pdc_screen;
PDCEX int pdc_yoffset;				  
#endif // EMSCRIPTEN

void init_curses(); // declaration of a function that handles curses procedures
Game game; // create a global game object

int main()
{
	// Memory leak detection (commented - enable only during debugging)
	// _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	// _CrtSetBreakAlloc(1779);
	
	// Debug logging setup - C++ Core Guidelines E.6: Exception-safe file handling
	std::ofstream debugFile;
	try {
		debugFile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
		debugFile.open("clog.txt"); // Will throw on failure - proper RAII
		std::clog.rdbuf(debugFile.rdbuf()); // redirect std::clog to the file
	}
	catch (const std::exception& e) {
		std::cerr << "Warning: Could not open debug file clog.txt: " << e.what() << std::endl;
		// Continue execution without debug logging
	}
	
	#ifdef EMSCRIPTEN
	//==INIT_SDL==
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "Could not initialize SDL: " << SDL_GetError() << std::endl;
		return 1;
	}
	atexit(SDL_Quit);

	/* Create window - size based on desired console dimensions */
	// For a 30x119 console, adjust window size accordingly
	pdc_window = SDL_CreateWindow("C++RogueLike",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		1190, 600, 0);  // Approximate size to accommodate 119x30 console
	if (!pdc_window) {
		std::cerr << "Could not create window: " << SDL_GetError() << std::endl;
		return 1;
	}
	pdc_screen = SDL_GetWindowSurface(pdc_window);
	pdc_yoffset = 0;  /* Use entire window for curses */
	
	/* Set environment variable to control PDCurses size */
	putenv((char*)"COLS=119");
	putenv((char*)"LINES=30");
	#endif	

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
	
	#ifdef EMSCRIPTEN
	// Clean up SDL resources
	SDL_DestroyWindow(pdc_window);
	#endif

	// C++ Core Guidelines E.6: Explicit close for error handling
	if (debugFile.is_open()) {
		try {
			debugFile.close();
			if (debugFile.fail()) {
				std::cerr << "Warning: Error closing debug file" << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Warning: Exception while closing debug file: " << e.what() << std::endl;
		}
	}
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