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
#include "Map/Decoration.h"
#include "Map/DungeonRoom.h"
#include "Map/Map.h"
#include "Map/Minimap.h"
#include "Menu/BaseMenu.h"
#include "Random/RandomDice.h"
#include "Renderer/InputSystem.h"
#include "Renderer/Renderer.h"
#include "Systems/AnimationSystem.h"
#include "Systems/BuffSystem.h"
#include "Systems/ContentRegistry.h"
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
#include "Systems/TileConfig.h"
#include "Utils/Dijkstra.h"
#include "Utils/Vector2D.h"

#ifndef EMSCRIPTEN
#include "Tools/ContentEditor.h"
#include "Tools/DecorEditor.h"
#include "Tools/ItemEditor.h"
#include "Tools/MonsterEditor.h"
#include "Tools/PrefabLibrary.h"
#include "Tools/RoomEditor.h"
#include "Tools/SpellEditor.h"
#endif

struct Game
{

	// Game state
	GameState game_state{};

	// Tile configuration (must be loaded before init_world)
	TileConfig tile_config{};

	// Minimap overlay
	Minimap minimap{};

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
	Dijkstra pathfinder{ get_map_width(), get_map_height() };
#ifndef EMSCRIPTEN
	ContentEditor content_editor{};
	DecorEditor decor_editor{};
	RoomEditor room_editor{};
	ItemEditor item_editor{};
	MonsterEditor monster_editor{};
	SpellEditor spell_editor{};
	PrefabLibrary prefab_library{};
#endif

	// Game world
	Map map{ get_map_width(), get_map_height() };
	Gui gui{};
	std::unique_ptr<Stairs> stairs{ std::make_unique<Stairs>(Vector2D{ 0, 0 }) };
	std::unique_ptr<Player> player{ nullptr };

	std::vector<DungeonRoom> rooms{};
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::vector<std::unique_ptr<Object>> objects{};
	std::vector<std::unique_ptr<Decoration>> decorations{};
	InventoryData inventory_data{ 1000 };

	// Character creation blueprint — persists across ticks
	PlayerBlueprint player_blueprint{};

	// Menu system
	std::deque<std::unique_ptr<BaseMenu>> menus{};

	// Mouse path overlay — persistent across frames, owned here
	std::vector<Vector2D> mouse_path_overlay{};

	[[nodiscard]] GameContext context() noexcept;
	bool tick(int& loopNum);
	void init_world();
	void shutdown();
};
