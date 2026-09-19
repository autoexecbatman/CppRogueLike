#pragma once
// file: Game.h
// Header-only game container - owns all systems, provides GameContext
// C++ Core Guidelines: C.9 minimize exposure, Rule of Zero

#include <deque>
#include <memory>
#include <vector>

#include "InventoryData.h"
#include "TileFeature.h"
#include "Player.h"
#include "GameContext.h"
#include "Gui.h"
#include "Decoration.h"
#include "DungeonRoom.h"
#include "Map.h"
#include "Minimap.h"
#include "BaseMenu.h"
#include "RandomDice.h"
#include "InputSystem.h"
#include "Renderer.h"
#include "AnimationSystem.h"
#include "BuffSystem.h"
#include "ContentRegistry.h"
#include "CreatureManager.h"
#include "DataManager.h"
#include "DisplayManager.h"
#include "FloatingTextSystem.h"
#include "GameLoopCoordinator.h"
#include "GameStateManager.h"
#include "HungerSystem.h"
#include "InputHandler.h"
#include "LevelManager.h"
#include "MenuManager.h"
#include "MessageSystem.h"
#include "RenderingManager.h"
#include "CurseSystem.h"
#include "TargetingSystem.h"
#include "SpellTile.h"
#include "Trap.h"
#include "BodyPlanRegistry.h"
#include "SpellRegistry.h"
#include "MonsterRegistry.h"
#include "TileConfig.h"
#include "Dijkstra.h"
#include "Vector2D.h"

#include "DecorEditor.h"
#include "PrefabLibrary.h"
#ifndef __EMSCRIPTEN__
#include "ContentEditor.h"
#include "ItemEditor.h"
#include "MonsterEditor.h"
#include "RoomEditor.h"
#include "SpellEditor.h"
#endif

struct Game
{

	// Game state
	GameState gameState{};

	// Tile configuration (must be loaded before init_world)
	TileConfig tileConfig{};
	BodyPlanRegistry bodyPlanRegistry{};

	// Every spell and every monster the game knows, loaded before init_world
	SpellRegistry spellRegistry{};
	MonsterRegistry monsterRegistry{};

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
#ifndef __EMSCRIPTEN__
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
	std::vector<std::unique_ptr<Trap>> traps{};
	std::vector<std::unique_ptr<SpellTile>> spellTiles{};
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
