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
		.playerBlueprint = &playerBlueprint,

		// Core systems
		.messageSystem = &messageSystem,
		.dice = &dice,

		// Managers
		.creatureManager = &creatureManager,
		.levelManager = &levelManager,
		.renderingManager = &renderingManager,
		.inputHandler = &inputHandler,
		.stateManager = &stateManager,
		.menuManager = &menuManager,
		.displayManager = &displayManager,
		.gameLoopCoordinator = &gameLoopCoordinator,
		.dataManager = &dataManager,

		// Rendering
		.renderer = &renderer,
		.inputSystem = &inputSystem,

		// Specialized systems
		.targeting = &targeting,
		.hungerSystem = &hungerSystem,
		.buffSystem = &buffSystem,
		.floatingText = &floatingText,
		.animSystem = &animSystem,
		.curseSystem = &curseSystem,
		.contentRegistry = &contentRegistry,
		.minimap = &minimap,
		.pathfinder = &pathfinder,

		.decorEditor = &decorEditor,
		.prefabLibrary = &prefabLibrary,
#ifndef EMSCRIPTEN
		.contentEditor = &contentEditor,
		.roomEditor = &roomEditor,
		.itemEditor = &itemEditor,
		.monsterEditor = &monsterEditor,
		.spellEditor = &spellEditor,
#endif
		.tileConfig = &tileConfig,

		// Game world data
		.stairs = stairs.get(),
		.objects = &objects,
		.decorations = &decorations,
		.floorInventory = &floorInventory,
		.creatures = &creatures,
		.rooms = &rooms,

		// UI
		.menus = &menus,

		// Mouse path overlay
		.mousePathOverlay = &mousePathOverlay,

		// Game state
		.gameState = &gameState
	};
}

// Game loop - returns false when game should exit
bool Game::tick(int& loopNum)
{
	if (!gameState.get_run())
	{
		return false;
	}

#ifndef EMSCRIPTEN
	if (roomEditor.is_active())
	{
		gameState.set_window_state(WindowState::ROOM_EDITOR);
	}
	else if (itemEditor.is_active())
	{
		gameState.set_window_state(WindowState::ITEM_EDITOR);
	}
	else if (monsterEditor.is_active())
	{
		gameState.set_window_state(WindowState::MONSTER_EDITOR);
	}
	else if (spellEditor.is_active())
	{
		gameState.set_window_state(WindowState::SPELL_EDITOR);
	}
	else
#endif
	{
		gameState.set_window_state(menus.empty() ? WindowState::GAME : WindowState::MENU);
	}

	auto ctx = context();
	switch (gameState.get_window_state())
	{

	case WindowState::MENU:
	{
		menuManager.handle_menus(menus, ctx);
		break;
	}

	case WindowState::GAME:
	{
		gameLoopCoordinator.handle_gameloop(ctx, gui, loopNum);
		break;
	}

#ifndef EMSCRIPTEN

	case WindowState::ROOM_EDITOR:
	{
		roomEditor.tick(ctx);
		break;
	}

	case WindowState::ITEM_EDITOR:
	{
		itemEditor.tick(ctx);
		break;
	}

	case WindowState::MONSTER_EDITOR:
	{
		monsterEditor.tick(ctx);
		break;
	}

	case WindowState::SPELL_EDITOR:
	{
		spellEditor.tick(ctx);
		break;
	}
#endif
	default:
	{
		throw std::logic_error("windowState is unknown.");
	}
	}

	++loopNum;
	return gameState.get_run();
}

// Set tile refs for objects constructed before tile_config was loaded.
// Call after tile_config.load().
void Game::init_world()
{
	stairs->actorData.tile = tileConfig.get("TILE_STAIRS");
	animSystem.init(tileConfig, renderer.get_tile_size());
}

// Save on exit if needed
void Game::shutdown()
{
	if (gameState.get_should_save())
	{
		try
		{
			auto ctx = context();
			stateManager.save_game(ctx);
		}
		catch (const std::exception& e)
		{
			messageSystem.log("Error saving: " + std::string(e.what()));
		}
	}
}
