#pragma once
// file: Game.h
// Header-only game container - owns all systems, provides GameContext
// C++ Core Guidelines: C.9 minimize exposure, Rule of Zero

#include <deque>
#include <memory>
#include <vector>

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
#include "Systems/CurseSystem.h"
#include "Systems/TargetingSystem.h"
#include "Systems/TileConfig.h"
#include "Utils/Dijkstra.h"
#include "Utils/Vector2D.h"

#include "Tools/DecorEditor.h"
#include "Tools/PrefabLibrary.h"
#ifndef EMSCRIPTEN
#include "Tools/ContentEditor.h"
#include "Tools/ItemEditor.h"
#include "Tools/MonsterEditor.h"
#include "Tools/RoomEditor.h"
#include "Tools/SpellEditor.h"
#endif

struct Game
{

	// Game state
	GameState gameState{};

	// Tile configuration (must be loaded before init_world)
	TileConfig tileConfig{};

	// Minimap overlay
	Minimap minimap{};

	// Rendering (raylib)
	Renderer renderer{};
	InputSystem inputSystem{};

	// Core systems
	RandomDice dice{};
	MessageSystem messageSystem{};
	RenderingManager renderingManager{};
	InputHandler inputHandler{};
	GameStateManager stateManager{};
	LevelManager levelManager{};
	CreatureManager creatureManager{};
	MenuManager menuManager{};
	DisplayManager displayManager{};
	GameLoopCoordinator gameLoopCoordinator{};
	DataManager dataManager{};
	TargetingSystem targeting{};
	HungerSystem hungerSystem{};
	BuffSystem buffSystem{};
	FloatingTextSystem floatingText{};
	AnimationSystem animSystem{};
	ContentRegistry contentRegistry{};
	Dijkstra pathfinder{ get_map_width(), get_map_height() };
	DecorEditor decorEditor{};
	PrefabLibrary prefabLibrary{};
	CurseSystem curseSystem{};
#ifndef EMSCRIPTEN
	ContentEditor contentEditor{};
	RoomEditor roomEditor{};
	ItemEditor itemEditor{};
	MonsterEditor monsterEditor{};
	SpellEditor spellEditor{};
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
	FloorInventory floorInventory{ 1000 };

	// Character creation blueprint — persists across ticks
	PlayerBlueprint playerBlueprint{};

	// Menu system
	std::deque<std::unique_ptr<BaseMenu>> menus{};

	// Mouse path overlay — persistent across frames, owned here
	std::vector<Vector2D> mousePathOverlay{};

	[[nodiscard]] GameContext context() noexcept;
	bool tick(int& loopNum);
	void init_world();
	void shutdown();
};
