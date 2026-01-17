// file: Systems/GameLoopCoordinator.cpp
#include <curses.h>
#include <algorithm>

#include "GameLoopCoordinator.h"
#include "CreatureManager.h"
#include "HungerSystem.h"
#include "LevelManager.h"
#include "../Core/GameContext.h"
#include "../Game.h"
#include "../Gui/Gui.h"
#include "../Map/Map.h"
#include "../ActorTypes/Player.h"
#include "../Actor/Destructible.h"
#include "../Colors/Colors.h"

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
        ctx.state_manager->init_new_game(ctx);
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
    ctx.game_loop_coordinator->update(ctx); // update map and actors positions
    gui.gui_update(ctx); // update the gui
    ctx.message_system->log("Update OK.");
}

void GameLoopCoordinator::handle_render_phase(GameContext& ctx, Gui& gui)
{
    //==DRAW==
    ctx.message_system->log("Running render...");
    // Render game content first, then GUI on top
    ctx.rendering_manager->render(ctx); // render map and actors to the screen
    // Render GUI if it's initialized - AFTER game render so it's not overwritten
    if (gui.guiInit) {
        // Ensure GUI has latest data before rendering
        gui.gui_update(ctx);
        gui.gui_render(ctx); // render the gui
    }
    // Call the same restore function that inventory uses
    ctx.rendering_manager->restore_game_display();
    ctx.message_system->log("Render OK.");
}

void GameLoopCoordinator::handle_menu_check(GameContext& ctx)
{
    // Check for menus AFTER rendering so positions are updated
    if (ctx.menu_manager->has_active_menus(*ctx.menus))
    {
        *ctx.window_state = WindowState::MENU;
        return;
    }
}

void GameLoopCoordinator::update(GameContext& ctx)
{
    if (*ctx.game_status == GameStatus::VICTORY)
    {
        ctx.message_system->log("Player has won the game!");
        ctx.message_system->append_message_part(RED_YELLOW_PAIR, "Congratulations!");
        ctx.message_system->append_message_part(WHITE_BLACK_PAIR, " You have obtained the ");
        ctx.message_system->append_message_part(RED_YELLOW_PAIR, "Amulet of Yendor");
        ctx.message_system->append_message_part(WHITE_BLACK_PAIR, " and escaped the dungeon!");
        ctx.message_system->finalize_message();

        WINDOW* victoryWin = newwin(10, 50, (LINES / 2) - 5, (COLS / 2) - 25);
        box(victoryWin, 0, 0);
        mvwprintw(victoryWin, 2, 10, "VICTORY!");
        mvwprintw(victoryWin, 4, 5, "You have won the game!");
        mvwprintw(victoryWin, 6, 5, "Press any key to exit...");
        wrefresh(victoryWin);
        getch();
        delwin(victoryWin);

        *ctx.run = false;
    }

    ctx.map->update();
    ctx.player->update(ctx);

    if (*ctx.game_status == GameStatus::STARTUP)
    {
        ctx.map->compute_fov(ctx);
        if (ctx.level_manager->get_dungeon_level() == 1 && !*ctx.isLoadedGame)
        {
            ctx.player->racial_ability_adjustments();
            ctx.player->equip_class_starting_gear(ctx);
        }
        *ctx.isLoadedGame = false;
        ctx.player->calculate_thaco();
        *ctx.game_status = GameStatus::NEW_TURN;

        if (!ctx.gui->guiInit)
        {
            ctx.gui->gui_init();
            ctx.gui->guiInit = true;
            ctx.gui->gui_update(ctx);
        }
    }

    if (*ctx.game_status == GameStatus::NEW_TURN)
    {
        std::erase_if(*ctx.objects, [](const auto& obj) { return !obj; });

        ctx.creature_manager->update_creatures(*ctx.creatures, ctx);
        ctx.creature_manager->spawn_creatures(*ctx.creatures, *ctx.rooms, *ctx.map, *ctx.dice, *ctx.time, ctx);

        for (const auto& creature : *ctx.creatures)
        {
            if (creature && creature->destructible)
            {
                creature->destructible->update_constitution_bonus(*creature, ctx);
            }
        }

        if (ctx.player && ctx.player->destructible)
        {
            ctx.player->destructible->update_constitution_bonus(*ctx.player, ctx);
        }

        ctx.hunger_system->increase_hunger(ctx, 1);
        ctx.hunger_system->apply_hunger_effects(ctx);

        (*ctx.time)++;
        if (*ctx.game_status != GameStatus::DEFEAT)
        {
            *ctx.game_status = GameStatus::IDLE;
        }
    }

    if (*ctx.game_status == GameStatus::DEFEAT)
    {
        ctx.message_system->log("Player is dead!");
        ctx.message_system->append_message_part(COLOR_RED, "You died! Press any key...");
        ctx.message_system->finalize_message();
        *ctx.run = false;
    }
}

// end of file: Systems/GameLoopCoordinator.cpp
