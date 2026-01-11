// file: Systems/GameLoopCoordinator.cpp
#include "GameLoopCoordinator.h"
#include "../Core/GameContext.h"
#include "../Game.h"
#include "../Gui/Gui.h"

void GameLoopCoordinator::handle_gameloop(GameContext& ctx, Gui& gui, int loopNum)
{
    handle_initialization(ctx);

    // Debug game loop tracking
    if (ctx.message_system->is_debug_mode()) {
        ctx.message_system->log("//====================LOOP====================//");
        ctx.message_system->log("Loop number: " + std::to_string(loopNum) + "\n");
    }

    //==INIT_GUI==
    // GUI initialization is now handled in STARTUP completion
    // This ensures it happens after racial bonuses are applied

    handle_input_phase(ctx);
    handle_update_phase(ctx, gui);
    handle_render_phase(ctx, gui);
    handle_menu_check(ctx);
}

void GameLoopCoordinator::handle_initialization(GameContext& ctx)
{
    if (!ctx.menu_manager->is_game_initialized())
    {
        ctx.game->init();
        ctx.menu_manager->set_game_initialized(true);
    }
}

void GameLoopCoordinator::handle_input_phase(GameContext& ctx)
{
    //==INPUT==
    ctx.input_handler->reset_key(); // reset the keyPress so it won't get stuck in a loop
    if (ctx.menu_manager->should_take_input())
    {
        ctx.input_handler->key_store();
        ctx.input_handler->key_listen();
    }
    ctx.menu_manager->set_should_take_input(true); // reset shouldInput to reset the flag
}

void GameLoopCoordinator::handle_update_phase(GameContext& ctx, Gui& gui)
{
    //==UPDATE==
    ctx.message_system->log("Running update...");
    ctx.game->update(ctx); // update map and actors positions
    gui.gui_update(ctx); // update the gui
    ctx.message_system->log("Update OK.");
}

void GameLoopCoordinator::handle_render_phase(GameContext& ctx, Gui& gui)
{
    //==DRAW==
    ctx.message_system->log("Running render...");
    // Render game content first, then GUI on top
    ctx.game->render(); // render map and actors to the screen
    // Render GUI if it's initialized - AFTER game render so it's not overwritten
    if (gui.guiInit) {
        // Ensure GUI has latest data before rendering
        gui.gui_update(ctx);
        gui.gui_render(ctx); // render the gui
    }
    // Call the same restore function that inventory uses
    ctx.game->restore_game_display();
    ctx.message_system->log("Render OK.");
}

void GameLoopCoordinator::handle_menu_check(GameContext& ctx)
{
    // Check for menus AFTER rendering so positions are updated
    if (ctx.menu_manager->has_active_menus(ctx.game->menus))
    {
        ctx.game->windowState = Game::WindowState::MENU;
        return;
    }
}

// end of file: Systems/GameLoopCoordinator.cpp
