// file: Systems/GameLoopCoordinator.cpp
#include <algorithm>
#include <cmath>
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

    // Center camera on player before rendering
    ctx.renderer->set_camera_center(
        ctx.player->position.x,
        ctx.player->position.y,
        ctx.map->get_width(),
        ctx.map->get_height()
    );

    ctx.renderer->begin_frame();

    ctx.rendering_manager->render(ctx);

    if (gui.guiInit)
    {
        gui.gui_render(ctx);
    }

    draw_hover_tooltip(ctx);

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

void GameLoopCoordinator::draw_hover_tooltip(GameContext& ctx)
{
    if (!ctx.renderer || !ctx.input_system || !ctx.map) return;
    int ts = ctx.renderer->get_tile_size();
    if (ts <= 0) return;

    Vector2D screen_tile = ctx.input_system->get_mouse_tile(ts);

    // Ignore cursor over the GUI panel rows at the bottom
    int map_rows = ctx.renderer->get_viewport_rows() - GUI_RESERVE_ROWS;
    if (screen_tile.y < 0 || screen_tile.y >= map_rows) return;
    if (screen_tile.x < 0 || screen_tile.x >= ctx.renderer->get_viewport_cols()) return;

    Vector2D world_tile
    {
        screen_tile.x + ctx.renderer->get_camera_x() / ts,
        screen_tile.y + ctx.renderer->get_camera_y() / ts
    };

    if (!ctx.map->is_in_bounds(world_tile)) return;
    if (!ctx.map->is_explored(world_tile)) return;

    bool in_fov = ctx.map->is_in_fov(world_tile);

    // Build description; pick highlight tint based on content
    std::string desc;
    switch (ctx.map->get_tile_type(world_tile))
    {
    case TileType::FLOOR:       desc = "Floor";       break;
    case TileType::WALL:        desc = "Wall";        break;
    case TileType::WATER:       desc = "Water";       break;
    case TileType::CLOSED_DOOR: desc = "Closed door"; break;
    case TileType::OPEN_DOOR:   desc = "Open door";   break;
    case TileType::CORRIDOR:    desc = "Corridor";    break;
    default:                    desc = "Unknown";     break;
    }

    // Tint: cyan (terrain), amber (creature), pale green (item)
    unsigned char hr = 0, hg = 220, hb = 255;

    if (in_fov)
    {
        Creature* actor = ctx.map->get_actor(world_tile, ctx);
        if (actor)
        {
            desc  = actor->actorData.name;
            hr = 255; hg = 180; hb = 0;    // amber
        }
        for (const auto& item : ctx.inventory_data->items)
        {
            if (item && item->position == world_tile)
            {
                desc += " [" + item->actorData.name + "]";
                if (!actor) { hr = 100; hg = 255; hb = 120; }   // pale green
                break;
            }
        }
    }

    // Pulse: 3 Hz sine, range [0, 1]
    float pulse = (std::sin(static_cast<float>(GetTime()) * 6.28318f * 3.0f) + 1.0f) * 0.5f;

    int   tx   = screen_tile.x * ts;
    int   ty   = screen_tile.y * ts;
    float tx_f = static_cast<float>(tx);
    float ty_f = static_cast<float>(ty);
    float ts_f = static_cast<float>(ts);

    // Inner fill: very subtle tint
    unsigned char fill_a = static_cast<unsigned char>(10 + static_cast<int>(15.0f * pulse));
    DrawRectangle(tx, ty, ts, ts, Color{ hr, hg, hb, fill_a });

    // Full perimeter: thin line, pulsing alpha
    unsigned char border_a = static_cast<unsigned char>(70 + static_cast<int>(80.0f * pulse));
    DrawRectangleLinesEx(Rectangle{ tx_f, ty_f, ts_f, ts_f }, 1.0f, Color{ hr, hg, hb, border_a });

    // Corner L-accents: bright, 2 px thick, clen px long
    int           clen     = ts / 4;
    int           clw      = 2;
    unsigned char corner_a = static_cast<unsigned char>(160 + static_cast<int>(95.0f * pulse));
    Color         cc       = Color{ hr, hg, hb, corner_a };

    // Top-left
    DrawRectangle(tx,              ty,              clen, clw,  cc);
    DrawRectangle(tx,              ty,              clw,  clen, cc);
    // Top-right
    DrawRectangle(tx + ts - clen,  ty,              clen, clw,  cc);
    DrawRectangle(tx + ts - clw,   ty,              clw,  clen, cc);
    // Bottom-left
    DrawRectangle(tx,              ty + ts - clw,   clen, clw,  cc);
    DrawRectangle(tx,              ty + ts - clen,  clw,  clen, cc);
    // Bottom-right
    DrawRectangle(tx + ts - clen,  ty + ts - clw,   clen, clw,  cc);
    DrawRectangle(tx + ts - clw,   ty + ts - clen,  clw,  clen, cc);

    // Tooltip box below (or above if near bottom)
    int font_off = (ts - ctx.renderer->get_font_size()) / 2;
    int text_w   = ctx.renderer->measure_text(desc);
    int pad      = ts / 4;
    int box_w    = text_w + pad * 2;
    int box_h    = ts;
    int tip_px   = tx;
    int tip_py   = ty + ts;

    if (tip_px + box_w > ctx.renderer->get_screen_width())
        tip_px = ctx.renderer->get_screen_width() - box_w;
    if (tip_py + box_h > ctx.renderer->get_screen_height())
        tip_py = ty - box_h;

    // Dark background + matching accent border on tooltip
    DrawRectangle(tip_px, tip_py, box_w, box_h, Color{ 8, 8, 16, 220 });
    DrawRectangleLinesEx(
        Rectangle{
            static_cast<float>(tip_px), static_cast<float>(tip_py),
            static_cast<float>(box_w),  static_cast<float>(box_h)
        },
        1.0f, Color{ hr, hg, hb, 180 }
    );
    ctx.renderer->draw_text(tip_px + pad, tip_py + font_off, desc, WHITE_BLACK_PAIR);
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

        ctx.creature_manager->cleanup_dead_creatures(*ctx.creatures);

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
