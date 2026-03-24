#include "Game.h"

// Single source of truth: build context from owned systems
[[nodiscard]] GameContext Game::context() noexcept
{
	return GameContext{
		// Core game world
		.map = &map,
		.gui = &gui,
		.player = player.get(),

		// Core systems
		.message_system = &message_system,
		.dice = &dice,

		// Managers
		.creature_manager = &creature_manager,
		.level_manager = &level_manager,
		.rendering_manager = &rendering_manager,
		.input_handler = &input_handler,
		.state_manager = &state_manager,
		.menu_manager = &menu_manager,
		.display_manager = &display_manager,
		.game_loop_coordinator = &game_loop_coordinator,
		.data_manager = &data_manager,

		// Rendering
		.renderer = &renderer,
		.input_system = &input_system,

		// Specialized systems
		.targeting = &targeting,
		.hunger_system = &hunger_system,
		.buff_system = &buff_system,
		.floating_text = &floating_text,
		.anim_system = &anim_system,
		.content_registry = &content_registry,
#ifndef EMSCRIPTEN
		.content_editor = &content_editor,
		.decor_editor = &decor_editor,
		.room_editor = &room_editor,
		.item_editor = &item_editor,
		.monster_editor = &monster_editor,
		.spell_editor = &spell_editor,
		.prefab_library = &prefab_library,
#endif
		.tile_config = &tile_config,

		// Game world data
		.stairs = stairs.get(),
		.objects = &objects,
		.inventory_data = &inventory_data,
		.creatures = &creatures,
		.rooms = &rooms,

		// UI
		.menus = &menus,

		// Game state
		.game_state = &game_state
	};
}

// Game loop - returns false when game should exit
bool Game::tick(int& loopNum)
{
	if (!game_state.get_run())
	{
		return false;
	}

#ifndef EMSCRIPTEN
	if (room_editor.is_active())
	{
		game_state.set_window_state(WindowState::ROOM_EDITOR);
	}
	else if (item_editor.is_active())
	{
		game_state.set_window_state(WindowState::ITEM_EDITOR);
	}
	else if (monster_editor.is_active())
	{
		game_state.set_window_state(WindowState::MONSTER_EDITOR);
	}
	else if (spell_editor.is_active())
	{
		game_state.set_window_state(WindowState::SPELL_EDITOR);
	}
	else
#endif
	{
		game_state.set_window_state(menus.empty() ? WindowState::GAME : WindowState::MENU);
	}

	auto ctx = context();
	switch (game_state.get_window_state())
	{

	case WindowState::MENU:
	{
		menu_manager.handle_menus(menus, ctx);
		break;
	}

	case WindowState::GAME:
	{
		game_loop_coordinator.handle_gameloop(ctx, gui, loopNum);
		break;
	}

#ifndef EMSCRIPTEN

	case WindowState::ROOM_EDITOR:
	{
		room_editor.tick(ctx);
		break;
	}

	case WindowState::ITEM_EDITOR:
	{
		item_editor.tick(ctx);
		break;
	}

	case WindowState::MONSTER_EDITOR:
	{
		monster_editor.tick(ctx);
		break;
	}

	case WindowState::SPELL_EDITOR:
	{
		spell_editor.tick(ctx);
		break;
	}
#endif
	default:
	{
		throw std::logic_error("windowState is unknown.");
	}
	}

	++loopNum;
	return game_state.get_run();
}

// Set tile refs for objects constructed before tile_config was loaded.
// Call after tile_config.load().
void Game::init_world()
{
	player->actorData.tile = tile_config.get("TILE_PLAYER");
	stairs->actorData.tile = tile_config.get("TILE_STAIRS");
	anim_system.init(tile_config, renderer.get_tile_size());
}

// Save on exit if needed
void Game::shutdown()
{
	if (game_state.get_should_save())
	{
		try
		{
			auto ctx = context();
			state_manager.save_game(ctx);
		}
		catch (const std::exception& e)
		{
			message_system.log("Error saving: " + std::string(e.what()));
		}
	}
}
