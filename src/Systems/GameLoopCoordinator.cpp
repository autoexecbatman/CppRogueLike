// file: Systems/GameLoopCoordinator.cpp
#include <cmath>
#include <format>
#include <string>
#include <vector>

#include <raylib.h>

#include "../Actor/Actor.h"
#include "../Actor/Destructible.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Core/Paths.h"
#include "../Gui/Gui.h"
#include "../Map/Map.h"
#include "../Map/Minimap.h"
#include "../Menu/DeathMenu.h"
#include "../Renderer/InputSystem.h"
#include "../Renderer/Renderer.h"
#include "../Systems/ContentRegistry.h"
#include "../Systems/GameStateManager.h"
#include "../Systems/InputHandler.h"
#include "../Systems/MenuManager.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/RenderingManager.h"
#include "../Tools/ContentEditor.h"
#include "../Tools/DecorEditor.h"
#include "../Tools/PrefabLibrary.h"
#include "../Utils/Vector2D.h"
#include "AnimationSystem.h"
#include "CreatureManager.h"
#include "FloatingTextSystem.h"
#include "GameLoopCoordinator.h"
#include "HungerSystem.h"
#include "LevelManager.h"

void GameLoopCoordinator::handle_gameloop(GameContext& ctx, Gui& gui, int loopNum)
{
	handle_initialization(ctx);

	if (ctx.messageSystem->is_debug_mode())
	{
		ctx.messageSystem->log("//====================LOOP====================//");
		ctx.messageSystem->log(std::format("Loop number: {}\n", loopNum));
	}

	handle_input_phase(ctx);

	if (ctx.inputHandler->was_resized())
	{
		ctx.inputHandler->clear_resize();
		ctx.renderer->update_viewport();
		ctx.map->compute_fov(ctx);

		if (ctx.gui->guiInit)
		{
			ctx.gui->gui_shutdown();
		}
		ctx.gui->gui_init();
		ctx.gui->guiInit = true;
		ctx.gui->gui_update(ctx);

		ctx.gameState->set_game_status(GameStatus::IDLE);
	}

	// Update game state only when player provides input and editor is not open
#ifndef EMSCRIPTEN
	const bool editor_active = (ctx.decorEditor && ctx.decorEditor->is_active()) ||
		(ctx.contentEditor && ctx.contentEditor->is_active());
#else
	const bool editor_active = false;
#endif
	// Path pacing: allow one update step every 0.12 s while auto-walking.
	// Keyboard input always bypasses the rate-limit (is_animation_tick() is false).
	bool has_mouse_path = ctx.mousePathOverlay && !ctx.mousePathOverlay->empty();
	bool pathStepReady = false;
	if (has_mouse_path)
	{
		double now = GetTime();
		if (now - mousePathStepTime >= 0.12)
		{
			pathStepReady = true;
			mousePathStepTime = now;
		}
	}
	if (!editor_active && (!ctx.inputHandler->is_animation_tick() || pathStepReady))
	{
		handle_update_phase(ctx, gui);
	}

	// Always render every frame
	handle_render_phase(ctx, gui);
	handle_menu_check(ctx);
}

void GameLoopCoordinator::handle_initialization(GameContext& ctx)
{
	// init_new_game is called by MenuName once the blueprint is complete.
	// load_all sets game_initialized directly. Nothing to do here.
}

void GameLoopCoordinator::handle_input_phase(GameContext& ctx)
{
	ctx.inputHandler->reset_key();
	if (ctx.menuManager->should_take_input())
	{
		ctx.inputSystem->poll();

		auto handle_zoom = [&]() -> bool
		{
			GameKey key = ctx.inputSystem->get_key();

			if (key == GameKey::MINIMAP_TOGGLE && ctx.minimap)
			{
				ctx.minimap->toggle();
				return true;
			}
			if (key == GameKey::ZOOM_IN)
			{
				ctx.renderer->zoom_in();
				ctx.map->compute_fov(ctx);
				return true;
			}
			if (key == GameKey::ZOOM_OUT)
			{
				ctx.renderer->zoom_out();
				ctx.map->compute_fov(ctx);
				return true;
			}
#ifndef EMSCRIPTEN
			if (key == GameKey::DECOR_EDIT_TOGGLE && ctx.decorEditor)
			{
				ctx.decorEditor->toggle();
				return true;
			}
			if (key == GameKey::CONTENT_EDIT_TOGGLE && ctx.contentEditor)
			{
				ctx.contentEditor->toggle(*ctx.contentRegistry);
				return true;
			}
			if (ctx.decorEditor && ctx.decorEditor->is_active())
			{
				if (key == GameKey::DECOR_PREV)
				{
					ctx.decorEditor->cycle_prev();
					return true;
				}
				if (key == GameKey::DECOR_NEXT)
				{
					ctx.decorEditor->cycle_next();
					return true;
				}
				if (key == GameKey::DECOR_SAVE)
				{
					ctx.decorEditor->save_palette(Paths::TILE_CONFIG);
					return true;
				}
				// Handle placement -- blocked while sheet browser is open
				if (!ctx.decorEditor->is_browser_open() && (key == GameKey::MOUSE_LEFT || key == GameKey::MOUSE_RIGHT))
				{
					Vector2D world = ctx.inputSystem->get_mouse_world_tile(
						ctx.renderer->get_camera_x(),
						ctx.renderer->get_camera_y(),
						ctx.renderer->get_tile_size());
					int world_x = world.x;
					int world_y = world.y;
					if (key == GameKey::MOUSE_LEFT)
					{
						ctx.decorEditor->place(world_x, world_y);
					}
					else
					{
						ctx.decorEditor->erase(world_x, world_y);
					}
					return true;
				}
			}
#endif
			return false;
		};

#ifndef EMSCRIPTEN
		// Editor active: consume all input -- game gets nothing.
		if (ctx.contentEditor && ctx.contentEditor->is_active())
		{
			handle_zoom();
			ctx.menuManager->set_should_take_input(true);
			return;
		}

		if (ctx.decorEditor && ctx.decorEditor->is_active())
		{
			handle_zoom();
			ctx.decorEditor->set_char_input(ctx.inputSystem->get_char_input());
			ctx.menuManager->set_should_take_input(true);
			return;
		}
#endif

		if (!handle_zoom())
		{
			ctx.inputHandler->key_store();
			ctx.inputHandler->key_listen(*ctx.inputSystem);
		}
	}
	ctx.menuManager->set_should_take_input(true);
}

void GameLoopCoordinator::handle_update_phase(GameContext& ctx, Gui& gui)
{
	ctx.messageSystem->log("Running update...");
	ctx.gameLoopCoordinator->update(ctx);
	gui.gui_update(ctx);
	ctx.messageSystem->log("Update OK.");
}

void GameLoopCoordinator::handle_render_phase(GameContext& ctx, Gui& gui)
{
	ctx.messageSystem->log("Running render...");

	// Center camera on player before rendering
	ctx.renderer->set_camera_center(
		ctx.player->position.x,
		ctx.player->position.y,
		ctx.map->get_width(),
		ctx.map->get_height());

	ctx.renderer->begin_frame();

	ctx.renderingManager->render(ctx);

	if (ctx.animSystem)
	{
		ctx.animSystem->update_and_render(*ctx.renderer);
	}

	if (ctx.floatingText)
	{
		ctx.floatingText->update_and_render(*ctx.renderer);
	}

#ifndef EMSCRIPTEN
	if (ctx.decorEditor)
	{
		ctx.decorEditor->update_and_render(*ctx.renderer);
	}

	if (ctx.contentEditor)
	{
		ctx.contentEditor->update_and_render(*ctx.renderer, *ctx.contentRegistry);
	}
#endif

	if (gui.guiInit)
	{
		gui.gui_render(ctx);
	}

	draw_hover_tooltip(ctx);

	ctx.renderer->end_frame();
	ctx.messageSystem->log("Render OK.");
}

void GameLoopCoordinator::handle_menu_check(GameContext& ctx)
{
	if (ctx.menuManager->has_active_menus(*ctx.menus))
	{
		ctx.gameState->set_window_state(WindowState::MENU);
		return;
	}
}

void GameLoopCoordinator::draw_hover_tooltip(GameContext& ctx)
{
	if (!ctx.renderer || !ctx.inputSystem || !ctx.map)
	{
		return;
	}

	int tileSize = ctx.renderer->get_tile_size();
	if (tileSize <= 0)
	{
		return;
	}

	int cam_x = ctx.renderer->get_camera_x();
	int cam_y = ctx.renderer->get_camera_y();

	// Correct world tile: combined division avoids split-integer rounding error
	// when cam_x is not tile-aligned (odd viewportCols on web).
	Vector2D world_tile = ctx.inputSystem->get_mouse_world_tile(cam_x, cam_y, tileSize);

	// Screen pixel position — same formula as tile rendering so the square
	// tracks the actual rendered tile regardless of camera alignment.
	int tx = world_tile.x * tileSize - cam_x;
	int ty = world_tile.y * tileSize - cam_y;

	// Ignore cursor over the GUI panel rows at the bottom
	int map_rows = ctx.renderer->get_viewport_rows() - GUI_RESERVE_ROWS;

	if (tx < 0 || ty < 0)
	{
		return;
	}
	if (ty / tileSize >= map_rows)
	{
		return;
	}
	if (tx / tileSize >= ctx.renderer->get_viewport_cols())
	{
		return;
	}

	if (!ctx.map->is_in_bounds(world_tile))
	{
		return;
	}
	if (!ctx.map->is_explored(world_tile))
	{
		return;
	}

	bool in_fov = ctx.map->is_in_fov(world_tile);

	// Build description; pick highlight tint based on content
	std::string desc;
	switch (ctx.map->get_tile_type(world_tile))
	{

	case TileType::FLOOR:
	{
		desc = "Floor";
		break;
	}

	case TileType::WALL:
	{
		desc = "Wall";
		break;
	}

	case TileType::WATER:
	{
		desc = "Water";
		break;
	}

	case TileType::CLOSED_DOOR:
	{
		desc = "Closed door";
		break;
	}

	case TileType::OPEN_DOOR:
	{
		desc = "Open door";
		break;
	}

	case TileType::CORRIDOR:
	{
		desc = "Corridor";
		break;
	}

	default:
	{
		desc = "Unknown";
		break;
	}
	}

	// Tint: cyan (terrain), amber (creature), pale green (item)
	unsigned char hr = 0, hg = 220, hb = 255;

	if (in_fov)
	{
		Creature* actor = ctx.map->get_actor(world_tile, ctx);
		if (actor)
		{
			desc = actor->actorData.name;
			hr = 255;
			hg = 180;
			hb = 0; // amber
		}
		for (const auto& item : ctx.inventoryData->items)
		{
			if (item && item->position == world_tile)
			{
				desc += " [" + item->actorData.name + "]";
				if (!actor)
				{
					hr = 100;
					hg = 255;
					hb = 120;
				} // pale green
				break;
			}
		}
	}

	// Pulse: 3 Hz sine, range [0, 1]
	float pulse = (std::sin(static_cast<float>(GetTime()) * 6.28318f * 3.0f) + 1.0f) * 0.5f;

	float tx_f = static_cast<float>(tx);
	float ty_f = static_cast<float>(ty);
	float ts_f = static_cast<float>(tileSize);

	// Inner fill: very subtle tint
	unsigned char fill_a = static_cast<unsigned char>(10 + static_cast<int>(15.0f * pulse));
	DrawRectangle(tx, ty, tileSize, tileSize, Color{ hr, hg, hb, fill_a });

	// Full perimeter: thin line, pulsing alpha
	unsigned char border_a = static_cast<unsigned char>(70 + static_cast<int>(80.0f * pulse));
	DrawRectangleLinesEx(Rectangle{ tx_f, ty_f, ts_f, ts_f }, 1.0f, Color{ hr, hg, hb, border_a });

	// Corner L-accents: bright, 2 px thick, clen px long
	int clen = tileSize / 4;
	int clw = 2;
	unsigned char corner_a = static_cast<unsigned char>(160 + static_cast<int>(95.0f * pulse));
	Color cc = Color{ hr, hg, hb, corner_a };

	// Top-left
	DrawRectangle(tx, ty, clen, clw, cc);
	DrawRectangle(tx, ty, clw, clen, cc);
	// Top-right
	DrawRectangle(tx + tileSize - clen, ty, clen, clw, cc);
	DrawRectangle(tx + tileSize - clw, ty, clw, clen, cc);
	// Bottom-left
	DrawRectangle(tx, ty + tileSize - clw, clen, clw, cc);
	DrawRectangle(tx, ty + tileSize - clen, clw, clen, cc);
	// Bottom-right
	DrawRectangle(tx + tileSize - clen, ty + tileSize - clw, clen, clw, cc);
	DrawRectangle(tx + tileSize - clw, ty + tileSize - clen, clw, clen, cc);

	// Tooltip box below (or above if near bottom)
	int font_off = (tileSize - ctx.renderer->get_font_size()) / 2;
	int text_w = ctx.renderer->measure_text(desc);
	int pad = tileSize / 4;
	int box_w = text_w + pad * 2;
	int box_h = tileSize;
	int tip_px = tx;
	int tip_py = ty + tileSize;

	if (tip_px + box_w > ctx.renderer->get_screen_width())
	{
		tip_px = ctx.renderer->get_screen_width() - box_w;
	}

	if (tip_py + box_h > ctx.renderer->get_screen_height())
	{
		tip_py = ty - box_h;
	}

	// Dark background + matching accent border on tooltip
	DrawRectangle(tip_px, tip_py, box_w, box_h, Color{ 8, 8, 16, 220 });
	DrawRectangleLinesEx(
		Rectangle{
			static_cast<float>(tip_px), static_cast<float>(tip_py), static_cast<float>(box_w), static_cast<float>(box_h) },
		1.0f,
		Color{ hr, hg, hb, 180 });
	ctx.renderer->draw_text(Vector2D{ tip_px + pad, tip_py + font_off }, desc, WHITE_BLACK_PAIR);
}

void GameLoopCoordinator::update(GameContext& ctx)
{
	if (ctx.gameState->get_game_status() == GameStatus::VICTORY)
	{
		ctx.messageSystem->log("Player has won the game!");
		ctx.messageSystem->append_message_part(RED_YELLOW_PAIR, "Congratulations!");
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " You have obtained the ");
		ctx.messageSystem->append_message_part(RED_YELLOW_PAIR, "Amulet of Yendor");
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " and escaped the dungeon!");
		ctx.messageSystem->finalize_message();
		ctx.gameState->set_run(false);
	}

	ctx.map->update();
	ctx.player->update(ctx);

	if (ctx.gameState->get_game_status() == GameStatus::STARTUP)
	{
#ifndef EMSCRIPTEN
		// Ensure active map key is set for loaded games (new games set it in Map::init).
		if (ctx.decorEditor && ctx.map && ctx.levelManager)
		{
			ctx.decorEditor->set_active_map(
				ctx.map->get_seed(),
				ctx.levelManager->get_dungeon_level());
		}
#endif

		ctx.map->compute_fov(ctx);
		ctx.map->update(); // stamp explored for the freshly-computed FOV so minimap is correct this frame
		if (ctx.levelManager->get_dungeon_level() == 1 && !ctx.gameState->get_is_loaded_game())
		{
			ctx.player->racial_ability_adjustments();
			ctx.player->equip_class_starting_gear(ctx);
		}
		bool wasLoadedGame = ctx.gameState->get_is_loaded_game();
		ctx.gameState->set_is_loaded_game(false);
		ctx.player->calculate_thaco();

		if (wasLoadedGame)
		{
			ctx.gameState->set_game_status(GameStatus::IDLE);
		}
		else
		{
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		}

		if (!ctx.gui->guiInit)
		{
			ctx.gui->gui_init();
			ctx.gui->guiInit = true;
			ctx.gui->gui_update(ctx);
		}
	}

	if (ctx.gameState->get_game_status() == GameStatus::NEW_TURN)
	{
		std::erase_if(*ctx.objects,
			[](const auto& obj)
			{ return !obj; });

		if (ctx.decorations)
		{
			std::erase_if(*ctx.decorations,
				[](const auto& d)
				{ return !d || d->isBroken; });
		}

		ctx.creatureManager->update_creatures(*ctx.creatures, ctx);
		ctx.creatureManager->spawn_creatures(ctx);

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

		ctx.hungerSystem->increase_hunger(ctx, 1);
		ctx.hungerSystem->apply_hunger_effects(ctx);

		ctx.creatureManager->cleanup_dead_creatures(*ctx.creatures);

		ctx.gameState->increment_time();
		if (ctx.gameState->get_game_status() != GameStatus::DEFEAT)
		{
			ctx.gameState->set_game_status(GameStatus::IDLE);
		}
	}

	if (ctx.gameState->get_game_status() == GameStatus::DEFEAT)
	{
		ctx.messageSystem->log("Player is dead!");
		ctx.menus->push_back(std::make_unique<DeathMenu>(ctx));
		ctx.gameState->set_game_status(GameStatus::IDLE);
	}
}
