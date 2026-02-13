// file: main.cpp
#include <iostream>
#include <fstream>

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include "Game.h"
#include "Menu/Menu.h"

#ifdef EMSCRIPTEN
struct LoopData
{
    Game* game;
    int loopNum;
};

void emscripten_loop(void* arg)
{
    auto* data = static_cast<LoopData*>(arg);
    if (!data->game->run)
    {
        emscripten_cancel_main_loop();
        return;
    }

    data->game->tick(data->loopNum);
}
#endif

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

    // Game owns everything including Renderer and InputSystem
    Game game;

    // Initialize raylib window
    game.renderer.init(119, 30);
    game.renderer.load_tilesets("tileset.bmp", "tileset1.bmp");

    auto ctx = game.context();
    game.menus.push_back(std::make_unique<Menu>(true, ctx));

    int loopNum{ 0 };

#ifdef EMSCRIPTEN
    LoopData loopData{ &game, loopNum };
    emscripten_set_main_loop_arg(emscripten_loop, &loopData, 60, 1);
#else
    // Frame-based game loop
    // Menus and game loop manage their own begin_frame/end_frame internally.
    // Main loop just drives tick(). Input polling happens inside each subsystem.
    while (!WindowShouldClose() && game.run)
    {
        game.tick(loopNum);
    }
#endif

    // Shutdown
    game.shutdown();
    game.renderer.shutdown();

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
