// file: main_C++RogueLike.cpp
// Clean entry point using header-only Game

#include <stdlib.h>
#include <iostream>
#include <fstream>

#ifdef EMSCRIPTEN
#define SDL_MAIN_HANDLED
#endif

#include <curses.h>

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

    // Set initial window size for PDCurses SDL2 (both native and web)
    // PDCurses SDL2 reads PDC_COLS/PDC_LINES when it creates its own window
    putenv((char*)"PDC_COLS=119");
    putenv((char*)"PDC_LINES=30");

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
