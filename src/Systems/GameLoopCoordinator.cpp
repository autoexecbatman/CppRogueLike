// file: Systems/GameLoopCoordinator.cpp
#include <algorithm>
#include <format>

#include "GameLoopCoordinator.h"
#include "CreatureManager.h"
#include "HungerSystem.h"
#include "LevelManager.h"
#include "../Core/GameContext.h"
#include "../Gui/Gui.h"
#include "../Map/Map.h"
#include "../ActorTypes/Player.h"
#include "../Actor/Destructible.h"
#include "../Colors/Colors.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/MenuManager.h"
#include "../Systems/InputHandler.h"
#include "../Systems/RenderingManager.h"
#include "../Systems/GameStateManager.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/InputSystem.h"

void GameLoopCoordinator::handle_gameloop(GameContext& ctx, Gui& gui, int loopNum)
{
    handle_initialization(ctx);

    if (ctx.message_system->is_debug_mode())
    {
        ctx.message_system->log("//====================LOOP====================//");
        ctx.message_system->log(std::format("Loop number: {}\n", loopNum));
    }

    handle_input_phase(ctx);

    if (ctx.input_handler->was_resized())
    {
        ctx.input_handler->clear_resize();
        ctx.map->regenerate(ctx);
        ctx.map->compute_fov(ctx);

        if (ctx.gui->guiInit)
        {
            ctx.gui->gui_shutdown();
        }
        ctx.gui->gui_init();
        ctx.gui->guiInit = true;
        ctx.gui->gui_update(ctx);

        *ctx.game_status = GameStatus::IDLE;
    }

    // Update game state only when player provides input
    if (!ctx.input_handler->is_animation_tick())
    {
        handle_update_phase(ctx, gui);
    }

    // Always render every frame
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
    ctx.input_handler->reset_key();
    if (ctx.menu_manager->should_take_input())
    {
        ctx.input_system->poll();
        ctx.input_handler->key_store();
        ctx.input_handler->key_listen(*ctx.input_system);
    }
    ctx.menu_manager->set_should_take_input(true);
}

void GameLoopCoordinator::handle_update_phase(GameContext& ctx, Gui& gui)
{
    ctx.message_system->log("Running update...");
    ctx.game_loop_coordinator->update(ctx);
    gui.gui_update(ctx);
    ctx.message_system->log("Update OK.");
}

void GameLoopCoordinator::handle_render_phase(GameContext& ctx, Gui& gui)
{
    ctx.message_system->log("Running render...");
    ctx.renderer->begin_frame();

    ctx.rendering_manager->render(ctx);

    if (gui.guiInit)
    {
        gui.gui_render(ctx);
    }

    ctx.renderer->end_frame();
    ctx.message_system->log("Render OK.");
}

void GameLoopCoordinator::handle_menu_check(GameContext& ctx)
{
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
        bool wasLoadedGame = *ctx.isLoadedGame;
        *ctx.isLoadedGame = false;
        ctx.player->calculate_thaco();

        if (wasLoadedGame)
        {
            *ctx.game_status = GameStatus::IDLE;
        }
        else
        {
            *ctx.game_status = GameStatus::NEW_TURN;
        }

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
        ctx.creature_manager->spawn_creatures(
            *ctx.creatures,
            *ctx.rooms,
            *ctx.map,
            *ctx.dice,
            *ctx.time,
            ctx
        );

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
        ctx.message_system->append_message_part(RED_BLACK_PAIR, "You died! Press any key...");
        ctx.message_system->finalize_message();
        *ctx.run = false;
    }
}
