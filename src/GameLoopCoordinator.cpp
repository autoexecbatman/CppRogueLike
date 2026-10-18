// file: GameLoopCoordinator.cpp
#include <cassert>
#include <cmath>
#include <format>
#include <iostream>
#include <string>
#include <vector>

#include <raylib.h>

#include "Actor.h"
#include "AnimationSystem.h"
#include "Colors.h"
#include "ContentEditor.h"
#include "Creature.h"
#include "CreatureManager.h"
#include "DeathMenu.h"
#include "DecorEditor.h"
#include "FloatingTextSystem.h"
#include "GameContext.h"
#include "GameLoopCoordinator.h"
#include "TurnSchedule.h"
#include "GameStateManager.h"
#include "Gui.h"
#include "HungerSystem.h"
#include "InputHandler.h"
#include "InputSystem.h"
#include "LevelManager.h"
#include "Map.h"
#include "MenuManager.h"
#include "MessageSystem.h"
#include "Minimap.h"
#include "NotificationMenu.h"
#include "Paths.h"
#include "Player.h"
#include "Renderer.h"
#include "RenderingManager.h"
#include "SpellTile.h"
#include "TileConfig.h"
#include "TileFeature.h"
#include "Trap.h"
#include "Vector2D.h"

// One pass of the game loop. The game is already initialised by the time this
// runs: MenuName calls init_new_game once the blueprint is complete, and
// load_all sets the initialised flag itself.
void GameLoopCoordinator::handle_gameloop(GameContext& ctx, Gui& gui, int loopNum)
{
	assert(ctx.mousePathOverlay && "GameLoopCoordinator::handle_gameloop: called without a mousePathOverlay");
	assert(ctx.inputHandler && "GameLoopCoordinator::handle_gameloop: called without an inputHandler");
	assert(ctx.gui && "GameLoopCoordinator::handle_gameloop: called without a gui");
	assert(ctx.map && "GameLoopCoordinator::handle_gameloop: called without a map");

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
#ifndef __EMSCRIPTEN__
	assert(ctx.decorEditor && "GameLoopCoordinator::handle_gameloop: called without a decorEditor");
	assert(ctx.contentEditor && "GameLoopCoordinator::handle_gameloop: called without a contentEditor");
	const bool editor_active = ctx.decorEditor->is_active() || ctx.contentEditor->is_active();
#else
	const bool editor_active = false;
#endif
	// Path pacing: allow one update step every 0.12 s while auto-walking.
	// Keyboard input always bypasses the rate-limit (is_animation_tick() is false).
	bool has_mouse_path = !ctx.mousePathOverlay->empty();
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

void GameLoopCoordinator::handle_input_phase(GameContext& ctx)
{
	assert(ctx.minimap && "GameLoopCoordinator::handle_input_phase: called without a minimap");
	assert(ctx.menuManager && "GameLoopCoordinator::handle_input_phase: called without a menuManager");
	assert(ctx.inputHandler && "GameLoopCoordinator::handle_input_phase: called without an inputHandler");
	assert(ctx.inputSystem && "GameLoopCoordinator::handle_input_phase: called without an inputSystem");
	assert(ctx.contentRegistry && "GameLoopCoordinator::handle_input_phase: called without a contentRegistry");
	assert(ctx.itemRegistry && "GameLoopCoordinator::handle_input_phase: called without an itemRegistry");
	assert(ctx.monsterRegistry && "GameLoopCoordinator::handle_input_phase: called without a monsterRegistry");
	assert(ctx.map && "GameLoopCoordinator::handle_input_phase: called without a map");

	ctx.inputHandler->reset_key();
	if (ctx.menuManager->should_take_input())
	{
		ctx.inputSystem->poll();

		auto handle_zoom = [&]() -> bool
		{
			GameKey key = ctx.inputSystem->get_key();

			if (key == GameKey::MINIMAP_TOGGLE)
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
#ifndef __EMSCRIPTEN__
			assert(ctx.decorEditor && "GameLoopCoordinator::handle_input_phase: called without a decorEditor");
			if (key == GameKey::DECOR_EDIT_TOGGLE)
			{
				ctx.decorEditor->toggle();
				return true;
			}
			assert(ctx.contentEditor && "GameLoopCoordinator::handle_input_phase: called without a contentEditor");
			if (key == GameKey::CONTENT_EDIT_TOGGLE)
			{
				ctx.contentEditor->toggle(*ctx.contentRegistry, *ctx.monsterRegistry, *ctx.itemRegistry);
				return true;
			}
			if (ctx.decorEditor->is_active())
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
					if (key == GameKey::MOUSE_LEFT)
					{
						ctx.decorEditor->place(world);
					}
					else
					{
						ctx.decorEditor->erase(world);
					}
					return true;
				}
			}
#endif
			return false;
		};

#ifndef __EMSCRIPTEN__
		// Editor active: consume all input -- game gets nothing.
		assert(ctx.contentEditor && "GameLoopCoordinator::handle_input_phase: called without a contentEditor");
		if (ctx.contentEditor->is_active())
		{
			handle_zoom();
			ctx.menuManager->set_should_take_input(true);
			return;
		}

		assert(ctx.decorEditor && "GameLoopCoordinator::handle_input_phase: called without a decorEditor");
		if (ctx.decorEditor->is_active())
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
	assert(ctx.gameLoopCoordinator && "GameLoopCoordinator::handle_update_phase: called without a gameLoopCoordinator");

	ctx.messageSystem->log("Running update...");
	ctx.gameLoopCoordinator->update(ctx);
	gui.gui_update(ctx);
	ctx.messageSystem->log("Update OK.");
}

void GameLoopCoordinator::handle_render_phase(GameContext& ctx, Gui& gui)
{
	assert(ctx.renderingManager && "GameLoopCoordinator::handle_render_phase: called without a renderingManager");
	assert(ctx.contentRegistry && "GameLoopCoordinator::handle_render_phase: called without a contentRegistry");
	assert(ctx.monsterRegistry && "GameLoopCoordinator::handle_render_phase: called without a monsterRegistry");
	assert(ctx.map && "GameLoopCoordinator::handle_render_phase: called without a map");

	ctx.messageSystem->log("Running render...");

	// Center camera on player before rendering
	ctx.renderer->set_camera_center(
		ctx.player()->position.x,
		ctx.player()->position.y,
		ctx.map->get_width(),
		ctx.map->get_height());

	ctx.renderingManager->render(ctx);

	assert(ctx.animSystem && "GameLoopCoordinator::handle_render_phase: called without an animSystem");
	ctx.animSystem->update_and_render(*ctx.renderer);

	assert(ctx.floatingText && "render: a frame was drawn with no floating text system");
	ctx.floatingText->update_and_render(*ctx.renderer);

#ifndef __EMSCRIPTEN__
	assert(ctx.decorEditor && "GameLoopCoordinator::handle_render_phase: called without a decorEditor");
	ctx.decorEditor->update_and_render(*ctx.renderer);

	assert(ctx.contentEditor && "GameLoopCoordinator::handle_render_phase: called without a contentEditor");
	ctx.contentEditor->update_and_render(*ctx.renderer, *ctx.contentRegistry, *ctx.monsterRegistry);
#endif

	if (gui.guiInit)
	{
		gui.gui_render(ctx);
	}

	draw_hover_tooltip(ctx);

	ctx.messageSystem->log("Render OK.");
}

void GameLoopCoordinator::handle_menu_check(GameContext& ctx)
{
	assert(ctx.menuManager && "GameLoopCoordinator::handle_menu_check: called without a menuManager");

	if (ctx.menuManager->has_active_menus(*ctx.menus))
	{
		ctx.gameState->set_window_state(WindowState::MENU);
		return;
	}
}

void GameLoopCoordinator::draw_hover_tooltip(GameContext& ctx)
{
	assert(ctx.tileConfig && "GameLoopCoordinator::draw_hover_tooltip: called without a tileConfig");
	assert(ctx.floorInventory && "GameLoopCoordinator::draw_hover_tooltip: called without a floorInventory");
	assert(ctx.map && "GameLoopCoordinator::draw_hover_tooltip: called without a map");
	assert(ctx.inputSystem && "GameLoopCoordinator::draw_hover_tooltip: called without an inputSystem");
	if (!ctx.renderer)
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

	// Screen pixel position - same formula as tile rendering so the square
	// tracks the actual rendered tile regardless of camera alignment.
	int tx = world_tile.x * tileSize - cam_x;
	int ty = world_tile.y * tileSize - cam_y;

	// Ignore cursor over the GUI panel rows at the bottom
	int map_rows = ctx.renderer->get_viewport_rows() - ctx.renderer->get_gui_reserve_rows();

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
	desc = ctx.tileConfig->get_tile_definition(ctx.map->get_tile_type(world_tile)).displayName;

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
		assert(std::ranges::none_of(ctx.floorInventory->items, [](const auto& i)
			{ return !i; }));
		for (const auto& item : ctx.floorInventory->items)
		{
			if (item->position == world_tile)
			{
				desc += " [" + item->get_name() + "]";
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
	ctx.renderer->draw_text(Vector2D{ tip_px + pad, tip_py + font_off }, desc, ColorPairId::WHITE_BLACK);
}

void GameLoopCoordinator::apply_round_upkeep(GameContext& ctx)
{
	assert(ctx.dataManager && "GameLoopCoordinator::apply_round_upkeep: called without a dataManager");
	assert(ctx.creatureManager && "GameLoopCoordinator::apply_round_upkeep: called without a creatureManager");
	assert(ctx.creatures && "GameLoopCoordinator::apply_round_upkeep: called without creatures");

	// A round runs only after update_creatures, whose AI needs a player; the rest are Game's value members.
	assert(ctx.player() && "apply_round_upkeep: a round ran with no player");
	assert(ctx.hungerSystem && "apply_round_upkeep: a round ran with no hunger system");

	// Rounds owed. One action may finish none or several, and each is fed once,
	// so a creature acting twice in a round does not feel it twice.
	const int roundsFinished = rounds_completed_at(ctx.gameState->get_time());
	if (ctx.gameState->get_rounds_run() >= roundsFinished)
	{
		ctx.creatureManager->cleanup_dead_creatures(*ctx.creatures);
		return;
	}

	// One place for what a round does to a body, so the player and the creatures cannot
	// drift apart - they were two copies of these four calls.
	const auto feel_the_round = [&ctx](Creature& creature, int thisRound)
	{
		// The clock reading this round ends at, which is what the clock-held effects read,
		// so every round of a long action is held against its own time.
		const int roundEndTime = thisRound * TIME_UNITS_PER_ROUND;
		creature.update_constitution_bonus(ctx);
		creature.regenerate_from_constitution(thisRound, *ctx.dataManager);
		creature.regenerate_from_ring(thisRound);
		creature.tick_poison(roundEndTime, ctx);
		creature.tick_breath(roundEndTime, ctx);
	};

	// Every round that finished is fed, in order and under its own number, so an action
	// of several rounds pays for each of them.
	for (int thisRound = ctx.gameState->get_rounds_run() + 1; thisRound <= roundsFinished; ++thisRound)
	{
		for (const auto& creature : *ctx.creatures)
		{
			assert(creature && "apply_round_upkeep: the creature list holds a null entry");
			feel_the_round(*creature, thisRound);
		}

		// The player is not in that list, so the round has to reach them on its own.
		feel_the_round(*ctx.player(), thisRound);

		ctx.hungerSystem->increase_hunger(ctx, 1);
		ctx.hungerSystem->apply_hunger_effects(ctx);

		// Erased at the end of the round they died in, so the dead are fed no later round.
		ctx.creatureManager->cleanup_dead_creatures(*ctx.creatures);

		ctx.gameState->set_rounds_run(thisRound);

		// The player is never erased, so a dead player ends the rounds instead.
		if (ctx.player()->is_dead())
		{
			break;
		}
	}
}

void GameLoopCoordinator::update(GameContext& ctx)
{
	assert(ctx.gui && "GameLoopCoordinator::update: called without a gui");
	assert(ctx.traps && "GameLoopCoordinator::update: called without traps");
	assert(ctx.spellTiles && "GameLoopCoordinator::update: called without spellTiles");
	assert(ctx.creatureManager && "GameLoopCoordinator::update: called without a creatureManager");
	assert(ctx.creatures && "GameLoopCoordinator::update: called without creatures");

	ctx.map->update();
	ctx.player()->update(ctx);

	if (ctx.gameState->get_game_status() == GameStatus::STARTUP)
	{
#ifndef __EMSCRIPTEN__
		// Ensure active map key is set for loaded games (new games set it in Map::init).
		assert(ctx.levelManager && "GameLoopCoordinator::update: a loaded game resumed without a levelManager");
		assert(ctx.decorEditor && "GameLoopCoordinator::update: called without a decorEditor");
		assert(ctx.map && "GameLoopCoordinator::update: called without a map");
		ctx.decorEditor->set_active_map(
			ctx.map->get_seed(),
			ctx.levelManager->get_dungeon_level());
#endif

		ctx.map->compute_fov(ctx);
		ctx.map->update(); // stamp explored for the freshly-computed FOV so minimap is correct this frame
		if (ctx.levelManager->get_dungeon_level() == 1 && !ctx.gameState->get_is_loaded_game())
		{
			ctx.player_concrete().on_new_game_start(ctx);
		}
		bool wasLoadedGame = ctx.gameState->get_is_loaded_game();
		ctx.gameState->set_is_loaded_game(false);
		ctx.player_concrete().recalculate_combat_stats();

		if (wasLoadedGame)
		{
			ctx.gameState->set_game_status(GameStatus::IDLE);
		}
		else
		{
			// Opens the first turn directly: spend_player_action would charge the player for starting.
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
		// Both floor containers are swept the same way: a feature may destroy
		// itself from inside on_creature_enter, so it stays owned until here.
		const auto is_spent = [](const auto& feature)
		{
			return feature->is_destroyed();
		};
		std::erase_if(*ctx.traps, is_spent);
		std::erase_if(*ctx.spellTiles, is_spent);

		assert(ctx.decorations && "GameLoopCoordinator::update: called without decorations");
		std::erase_if(*ctx.decorations,
			[](const auto& d)
			{ return !d || d->isBroken; });

		ctx.creatureManager->update_creatures(*ctx.creatures, ctx);
		ctx.creatureManager->spawn_creatures(ctx);

		apply_round_upkeep(ctx);

		ctx.gameState->set_game_status(GameStatus::IDLE);
	}
}

void GameLoopCoordinator::open_end_of_run_screen(GameContext& ctx)
{
	assert(ctx.menus && "open_end_of_run_screen: called without a menu stack");
	assert(ctx.messageSystem && "open_end_of_run_screen: called without a messageSystem");

	switch (ctx.gameState->get_run_outcome())
	{

	case RunOutcome::UNDECIDED:
	{
		assert(false && "open_end_of_run_screen: the run has not ended");
		break;
	}

	case RunOutcome::VICTORY:
	{
		auto victoryMenu = std::make_unique<NotificationMenu>(
			"Victory!",
			std::vector<std::string>{
				"Congratulations, brave adventurer!",
				"You have retrieved the Amulet of Yendor",
				"and escaped the dungeon alive.",
				"",
				"Your legend will be remembered." },
			ctx);
		// Closing the victory screen ends the run.
		const auto end_the_run = [](GameContext& closeCtx)
		{
			closeCtx.gameState->set_run(false);
		};
		victoryMenu->set_on_close(end_the_run);
		ctx.menus->push_back(std::move(victoryMenu));
		break;
	}

	case RunOutcome::DEFEAT:
	{
		// The outcome is stored rather than read off the player, so the two are held together here.
		assert(ctx.player()->is_dead() && "open_end_of_run_screen: the run is lost while the player still has hit points");
		ctx.messageSystem->log("Player is dead!");
		// Permadeath removes the save; false means a save outlived its character and would be offered next run.
		if (!GameStateManager::delete_save_file())
		{
			std::clog << "[GameLoopCoordinator] the save file could not be deleted; it will outlive the character\n";
		}
		ctx.menus->push_back(std::make_unique<DeathMenu>(ctx));
		break;
	}
	}
}
