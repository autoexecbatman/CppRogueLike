// file: main_C++RogueLike.cpp
// C++ Core Guidelines compliant entry point - no God class

#include <stdlib.h>

#ifdef EMSCRIPTEN
#define SDL_MAIN_HANDLED
#include <SDL.h>
PDCEX SDL_Window* pdc_window;
PDCEX SDL_Surface* pdc_screen;
PDCEX int pdc_yoffset;
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <deque>
#include <curses.h>

#include "Core/GameContext.h"
#include "Map/Map.h"
#include "ActorTypes/Player.h"
#include "Actor/InventoryData.h"
#include "Gui/Gui.h"
#include "Menu/Menu.h"
#include "Menu/BaseMenu.h"
#include "Colors/Colors.h"
#include "Random/RandomDice.h"

#include "Systems/TargetingSystem.h"
#include "Systems/HungerSystem.h"
#include "Systems/MessageSystem.h"
#include "Systems/RenderingManager.h"
#include "Systems/InputHandler.h"
#include "Systems/GameStateManager.h"
#include "Systems/LevelManager.h"
#include "Systems/CreatureManager.h"
#include "Systems/MenuManager.h"
#include "Systems/DisplayManager.h"
#include "Systems/GameLoopCoordinator.h"
#include "Systems/DataManager.h"

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
    // Debug logging - C++ Core Guidelines E.6
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

    // ========================================
    // Own all systems directly - no God class
    // ========================================

    // Game state
    bool run{ true };
    bool shouldSave{ true };
    bool isLoadedGame{ false };
    int gameTime{ 0 };
    GameStatus gameStatus{ GameStatus::STARTUP };
    WindowState windowState{ WindowState::GAME };

    // Core systems
    RandomDice dice;
    MessageSystem message_system;
    RenderingManager rendering_manager;
    InputHandler input_handler;
    GameStateManager state_manager;
    LevelManager level_manager;
    CreatureManager creature_manager;
    MenuManager menu_manager;
    DisplayManager display_manager;
    GameLoopCoordinator game_loop_coordinator;
    DataManager data_manager;
    TargetingSystem targeting;
    HungerSystem hunger_system;

    // Game world
    Map map{ MAP_HEIGHT, MAP_WIDTH };
    Gui gui;
    auto stairs = std::make_unique<Stairs>(Vector2D{ 0, 0 });
    auto player = std::make_unique<Player>(Vector2D{ 0, 0 });

    std::vector<Vector2D> rooms;
    std::vector<std::unique_ptr<Creature>> creatures;
    std::vector<std::unique_ptr<Object>> objects;
    InventoryData inventory_data{ 1000 };

    // Menu system
    std::deque<std::unique_ptr<BaseMenu>> menus;

    // ========================================
    // Build GameContext
    // ========================================
    GameContext ctx;

    // Core game world
    ctx.map = &map;
    ctx.gui = &gui;
    ctx.player = player.get();

    // Core systems
    ctx.message_system = &message_system;
    ctx.dice = &dice;
    ctx.dice_roller = &dice;

    // Managers
    ctx.creature_manager = &creature_manager;
    ctx.level_manager = &level_manager;
    ctx.rendering_manager = &rendering_manager;
    ctx.input_handler = &input_handler;
    ctx.state_manager = &state_manager;
    ctx.menu_manager = &menu_manager;
    ctx.display_manager = &display_manager;
    ctx.game_loop_coordinator = &game_loop_coordinator;
    ctx.data_manager = &data_manager;

    // Specialized systems
    ctx.targeting = &targeting;
    ctx.targeting_system = &targeting;
    ctx.hunger_system = &hunger_system;

    // Game world data
    ctx.stairs = stairs.get();
    ctx.objects = &objects;
    ctx.inventory_data = &inventory_data;
    ctx.creatures = &creatures;
    ctx.rooms = &rooms;

    // UI
    ctx.menus = &menus;

    // Game state
    ctx.time = &gameTime;
    ctx.run = &run;
    ctx.shouldSave = &shouldSave;
    ctx.isLoadedGame = &isLoadedGame;
    ctx.game_status = &gameStatus;
    ctx.window_state = &windowState;

    // ========================================
    // Game loop
    // ========================================
    menus.push_back(std::make_unique<Menu>(true, ctx));
    int loopNum{ 0 };

    while (run)
    {
        windowState = menus.empty() ? WindowState::GAME : WindowState::MENU;

        switch (windowState)
        {
        case WindowState::MENU:
            ctx.menu_manager->handle_menus(menus, ctx);
            break;
        case WindowState::GAME:
            ctx.game_loop_coordinator->handle_gameloop(ctx, gui, loopNum);
            break;
        }
        loopNum++;
    }

    // ========================================
    // Shutdown
    // ========================================
    if (shouldSave)
    {
        try
        {
            state_manager.save_game(map, rooms, *player, *stairs, creatures,
                                    inventory_data, gui, hunger_system,
                                    level_manager, gameTime);
        }
        catch (const std::exception& e)
        {
            message_system.log("Error saving: " + std::string(e.what()));
        }
    }

    gui.gui_shutdown();
    endwin();

    if (isendwin())
    {
        message_system.log("Curses shutdown successfully.");
    }
    else
    {
        message_system.log("Curses shutdown failed.");
        return EXIT_FAILURE;
    }

#ifdef EMSCRIPTEN
    SDL_DestroyWindow(pdc_window);
#endif

    if (debugFile.is_open())
    {
        try
        {
            debugFile.close();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Warning: Exception closing debug file: " << e.what() << std::endl;
        }
    }

    return 0;
}
