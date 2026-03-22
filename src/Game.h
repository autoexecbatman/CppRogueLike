#pragma once
// file: Game.h
// Header-only game container - owns all systems, provides GameContext
// C++ Core Guidelines: C.9 minimize exposure, Rule of Zero

#include <deque>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "Actor/Actor.h"
#include "Actor/InventoryData.h"
#include "ActorTypes/Player.h"
#include "Core/GameContext.h"
#include "Gui/Gui.h"
#include "Map/DungeonRoom.h"
#include "Map/Map.h"
#include "Menu/BaseMenu.h"
#include "Random/RandomDice.h"
#include "Renderer/InputSystem.h"
#include "Renderer/Renderer.h"
#include "Systems/AnimationSystem.h"
#include "Systems/TileConfig.h"
#include "Systems/BuffSystem.h"
#include "Systems/CreatureManager.h"
#include "Systems/DataManager.h"
#include "Systems/DisplayManager.h"
#include "Systems/FloatingTextSystem.h"
#include "Systems/GameLoopCoordinator.h"
#include "Systems/GameStateManager.h"
#include "Systems/HungerSystem.h"
#include "Systems/InputHandler.h"
#include "Systems/LevelManager.h"
#include "Systems/MenuManager.h"
#include "Systems/MessageSystem.h"
#include "Systems/RenderingManager.h"
#include "Systems/TargetingSystem.h"
#include "Systems/ContentRegistry.h"
#include "Tools/ContentEditor.h"
#include "Tools/DecorEditor.h"
#include "Tools/ItemEditor.h"
#include "Tools/MonsterEditor.h"
#include "Tools/SpellEditor.h"
#include "Tools/PrefabLibrary.h"
#include "Tools/RoomEditor.h"
#include "Utils/Vector2D.h"

class Game
{
public:
	// Rule of Zero - compiler generates correct special members
	Game() = default;

	// Single source of truth: build context from owned systems
	[[nodiscard]] GameContext context() noexcept
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
			.content_editor = &content_editor,
			.decor_editor = &decor_editor,
			.room_editor = &room_editor,
			.item_editor = &item_editor,
			.monster_editor = &monster_editor,
			.spell_editor = &spell_editor,
			.prefab_library = &prefab_library,
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
			.time = &time,
			.run = &run,
			.shouldSave = &shouldSave,
			.isLoadedGame = &isLoadedGame,
			.game_status = &gameStatus,
			.window_state = &windowState
		};
	}

	// Game loop - returns false when game should exit
	bool tick(int& loopNum)
	{
		if (!run)
		{
			return false;
		}

		if (room_editor.is_active())
		{
			windowState = WindowState::ROOM_EDITOR;
		}
		else if (item_editor.is_active())
		{
			windowState = WindowState::ITEM_EDITOR;
		}
		else if (monster_editor.is_active())
		{
			windowState = WindowState::MONSTER_EDITOR;
		}
		else if (spell_editor.is_active())
		{
			windowState = WindowState::SPELL_EDITOR;
		}
		else
		{
			windowState = menus.empty() ? WindowState::GAME : WindowState::MENU;
		}

		auto ctx = context();
		switch (windowState)
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
		}

		++loopNum;
		return run;
	}

	// Set tile refs for objects constructed before tile_config was loaded.
	// Call after tile_config.load().
	void init_world()
	{
		player->actorData.tile = tile_config.get("TILE_PLAYER");
		stairs->actorData.tile = tile_config.get("TILE_STAIRS");
		anim_system.init(tile_config);
	}

	// Save on exit if needed
	void shutdown()
	{
		if (shouldSave)
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

	// Game state
	bool run{ true };
	bool shouldSave{ true };
	bool isLoadedGame{ false };
	int time{ 0 };
	GameStatus gameStatus{ GameStatus::STARTUP };
	WindowState windowState{ WindowState::GAME };

	// Tile configuration (must be loaded before init_world)
	TileConfig tile_config{};

	// Rendering (raylib)
	Renderer renderer{};
	InputSystem input_system{};

	// Core systems
	RandomDice dice{};
	MessageSystem message_system{};
	RenderingManager rendering_manager{};
	InputHandler input_handler{};
	GameStateManager state_manager{};
	LevelManager level_manager{};
	CreatureManager creature_manager{};
	MenuManager menu_manager{};
	DisplayManager display_manager{};
	GameLoopCoordinator game_loop_coordinator{};
	DataManager data_manager{};
	TargetingSystem targeting{};
	HungerSystem hunger_system{};
	BuffSystem buff_system{};
	FloatingTextSystem floating_text{};
	AnimationSystem anim_system{};
	ContentRegistry content_registry{};
	ContentEditor content_editor{};
	DecorEditor decor_editor{};
	RoomEditor room_editor{};
	ItemEditor item_editor{};
	MonsterEditor monster_editor{};
	SpellEditor spell_editor{};
	PrefabLibrary prefab_library{};

	// Game world
	Map map{ get_map_width(), get_map_height() };
	Gui gui{};
	std::unique_ptr<Stairs> stairs{ std::make_unique<Stairs>(Vector2D{ 0, 0 }) };
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 0, 0 }) };

	std::vector<DungeonRoom> rooms{};
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::vector<std::unique_ptr<Object>> objects{};
	InventoryData inventory_data{ 1000 };

	// Menu system
	std::deque<std::unique_ptr<BaseMenu>> menus{};
};
