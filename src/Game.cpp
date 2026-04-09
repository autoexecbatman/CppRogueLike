#include "Game.h"

// Single source of truth: build context from owned systems
[[nodiscard]] GameContext Game::context() noexcept
{
	return GameContext{
		// Core game world
		.map = &map,
		.gui = &gui,
		.player = player.get(),
		.playerOwner = &player,
		.playerBlueprint = &player_blueprint,

		// Core systems
		.messageSystem = &message_system,
		.dice = &dice,

		// Managers
		.creatureManager = &creature_manager,
		.levelManager = &level_manager,
		.renderingManager = &rendering_manager,
		.inputHandler = &input_handler,
		.stateManager = &state_manager,
		.menuManager = &menu_manager,
		.displayManager = &display_manager,
		.gameLoopCoordinator = &game_loop_coordinator,
		.dataManager = &data_manager,

		// Rendering
		.renderer = &renderer,
		.inputSystem = &input_system,

		// Specialized systems
		.targeting = &targeting,
		.hungerSystem = &hunger_system,
		.buffSystem = &buff_system,
		.floatingText = &floating_text,
		.animSystem = &anim_system,
		.contentRegistry = &content_registry,
		.minimap = &minimap,
		.pathfinder = &pathfinder,

#ifndef EMSCRIPTEN
		.contentEditor = &content_editor,
		.decorEditor = &decor_editor,
		.roomEditor = &room_editor,
		.itemEditor = &item_editor,
		.monsterEditor = &monster_editor,
		.spellEditor = &spell_editor,
		.prefabLibrary = &prefab_library,
#endif
		.tileConfig = &tile_config,

		// Game world data
		.stairs = stairs.get(),
		.objects = &objects,
		.decorations = &decorations,
		.inventoryData = &inventory_data,
		.creatures = &creatures,
		.rooms = &rooms,

		// UI
		.menus = &menus,

		// Mouse path overlay
		.mousePathOverlay = &mouse_path_overlay,

		// Game state
		.gameState = &game_state
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
