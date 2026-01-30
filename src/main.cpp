// file: main_C++RogueLike.cpp
// Clean entry point using header-only Game

#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <curses.h>
#ifdef EMSCRIPTEN
#define SDL_MAIN_HANDLED
#include <SDL.h>
PDCEX SDL_Window* pdc_window;
PDCEX SDL_Surface* pdc_screen;
PDCEX int pdc_yoffset;
#endif

#include "Game.h"
#include "Menu/Menu.h"
#include "Colors/Colors.h"

namespace {
    void init_curses()
    {
        initscr();

        if (!has_colors())
        {
            std::cerr << "Error: Terminal does not support colors." << std::endl;
        }
        else
        {
            start_color();
            Colors colors;
            colors.my_init_pair();
        }

        cbreak();
        noecho();
        curs_set(0);
        keypad(stdscr, true);

        if (has_mouse())
        {
            mousemask(ALL_MOUSE_EVENTS, nullptr);
        }
        else
        {
            std::cerr << "Warning: Mouse not supported in this terminal." << std::endl;
        }

        printw("Welcome to C++RogueLike!");
        printw("Console size: %d x %d", COLS, LINES);
        refresh();
    }
}

int main()
{
    // Debug logging
    std::ofstream debugFile;
    try
    {
        debugFile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        debugFile.open("clog.txt");
        std::clog.rdbuf(debugFile.rdbuf());
    }
    catch (const std::exception& e)
    {
        std::cerr << "Warning: Could not open debug file: " << e.what() << std::endl;
    }

#ifdef EMSCRIPTEN
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Could not initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }
    atexit(SDL_Quit);

    pdc_window = SDL_CreateWindow("C++RogueLike",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        1190, 600, 0);
    if (!pdc_window)
    {
        std::cerr << "Could not create window: " << SDL_GetError() << std::endl;
        return 1;
    }
    pdc_screen = SDL_GetWindowSurface(pdc_window);
    pdc_yoffset = 0;

    putenv((char*)"COLS=119");
    putenv((char*)"LINES=30");
#endif

    init_curses();

    // Game owns everything
    Game game;
    auto ctx = game.context();
    game.menus.push_back(std::make_unique<Menu>(true, ctx));

    // Game loop
    int loopNum{ 0 };
    while (game.tick(loopNum)) {}

    // Shutdown
    game.shutdown();
    game.gui.gui_shutdown();
    endwin();

    if (!isendwin())
    {
        game.message_system.log("Curses shutdown failed.");
        return EXIT_FAILURE;
    }

#ifdef EMSCRIPTEN
    SDL_DestroyWindow(pdc_window);
#endif

    if (debugFile.is_open())
    {
        try { debugFile.close(); }
        catch (const std::exception& e)
        {
            std::cerr << "Warning: Exception closing debug file: " << e.what() << std::endl;
        }
    }

    return 0;
}
