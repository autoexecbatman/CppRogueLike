// file: Systems/GameLoopCoordinator.cpp
#include "GameLoopCoordinator.h"
#include "../Game.h"
#include "../Gui/Gui.h"

void GameLoopCoordinator::handle_gameloop(Game& game, Gui& gui, int loopNum)
{
    handle_initialization(game);
    
    // Debug game loop tracking
    if (game.message_system.is_debug_mode()) {
        game.log("//====================LOOP====================//");
        game.log("Loop number: " + std::to_string(loopNum) + "\n");
    }

    //==INIT_GUI==
    // GUI initialization is now handled in STARTUP completion
    // This ensures it happens after racial bonuses are applied

    handle_input_phase(game);
    handle_update_phase(game, gui);
    handle_render_phase(game, gui);
    handle_menu_check(game);
}

void GameLoopCoordinator::handle_initialization(Game& game)
{
    if (!game.menu_manager.is_game_initialized())
    {
        game.init();
        game.menu_manager.set_game_initialized(true);
    }
}

void GameLoopCoordinator::handle_input_phase(Game& game)
{
    //==INPUT==
    game.input_handler.reset_key(); // reset the keyPress so it won't get stuck in a loop
    if (game.menu_manager.should_take_input())
    {
        game.input_handler.key_store();
        game.input_handler.key_listen();
    }
    game.menu_manager.set_should_take_input(true); // reset shouldInput to reset the flag
}

void GameLoopCoordinator::handle_update_phase(Game& game, Gui& gui)
{
    //==UPDATE==
    game.log("Running update...");
    game.update(); // update map and actors positions
    gui.gui_update(); // update the gui
    game.log("Update OK.");
}

void GameLoopCoordinator::handle_render_phase(Game& game, Gui& gui)
{
    //==DRAW==
    game.log("Running render...");
    // Render game content first, then GUI on top
    game.render(); // render map and actors to the screen
    // Render GUI if it's initialized - AFTER game render so it's not overwritten
    if (gui.guiInit) {
        // Ensure GUI has latest data before rendering
        gui.gui_update();
        gui.gui_render(); // render the gui
    }
    // Call the same restore function that inventory uses
    game.restore_game_display();
    game.log("Render OK.");
}

void GameLoopCoordinator::handle_menu_check(Game& game)
{
    // Check for menus AFTER rendering so positions are updated
    if (game.menu_manager.has_active_menus(game.menus))
    {
        game.windowState = Game::WindowState::MENU;
        return;
    }
}

// end of file: Systems/GameLoopCoordinator.cpp
