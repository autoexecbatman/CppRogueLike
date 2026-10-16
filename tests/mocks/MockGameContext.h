#pragma once
#include <cassert>
#include <deque>
#include <memory>
#include <vector>

#include "src/BaseMenu.h"
#include "src/BodyPlanRegistry.h"
#include "src/BuffSystem.h"
#include "src/ContentEditor.h"
#include "src/ContentRegistry.h"
#include "src/CostOverlay.h"
#include "src/Creature.h"
#include "src/CreatureManager.h"
#include "src/DataManager.h"
#include "src/DecorEditor.h"
#include "src/Decoration.h"
#include "src/DisplayManager.h"
#include "src/DungeonRoom.h"
#include "src/FloatingTextSystem.h"
#include "src/GameContext.h"
#include "src/GameLoopCoordinator.h"
#include "src/GameStateManager.h"
#include "src/HungerSystem.h"
#include "src/InventoryData.h"
#include "src/ItemEditor.h"
#include "src/ItemRegistry.h"
#include "src/LevelManager.h"
#include "src/Map.h"
#include "src/MenuManager.h"
#include "src/MessageSystem.h"
#include "src/MonsterEditor.h"
#include "src/MonsterRegistry.h"
#include "src/Pathfinding.h"
#include "src/Paths.h"
#include "src/Player.h"
#include "src/PrefabLibrary.h"
#include "src/RandomDice.h"
#include "src/RoomEditor.h"
#include "src/SpellEditor.h"
#include "src/SpellRegistry.h"
#include "src/SpellTile.h"
#include "src/Stairs.h"
#include "src/TargetingSystem.h"
#include "src/TileConfig.h"
#include "src/Trap.h"
#include "src/Vector2D.h"

struct MockGameContext
{
	RandomDice dice{};
	MessageSystem messages{};
	CreatureManager creature_mgr{};
	DataManager data_manager{};
	ContentRegistry content_registry{};
	FloorInventory inventory{};
	GameState game_state{};

	// Stateless, and every creature update reaches through it: Creature::update
	// asks it to retire whatever the clock has passed before the creature acts.
	BuffSystem buffs{};
	TileConfig tile_config{};
	BodyPlanRegistry body_plans{};
	SpellRegistry spellRegistry{};
	MonsterRegistry monsterRegistry{};
	ItemRegistry itemRegistry{};
	Pathfinding::Pathfinder pathfinder{};
	FloatingTextSystem floatingText{};

	// Every member the game's own context wires, so a test reaches the same objects play does.
	Map map{ get_map_width(), get_map_height() };
	std::unique_ptr<Player> player{};
	PlayerBlueprint playerBlueprint{};
	LevelManager levelManager{};
	GameStateManager stateManager{};
	MenuManager menuManager{};
	DisplayManager displayManager{};
	GameLoopCoordinator gameLoopCoordinator{};
	TargetingSystem targeting{};
	HungerSystem hungerSystem{};
	CostOverlay costOverlay{};
	PrefabLibrary prefabLibrary{};
	Stairs stairs{ Vector2D{ 0, 0 } };
	std::vector<std::unique_ptr<Trap>> traps{};
	std::vector<std::unique_ptr<SpellTile>> spellTiles{};
	std::vector<std::unique_ptr<Decoration>> decorations{};
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::vector<DungeonRoom> rooms{};
	std::deque<std::unique_ptr<BaseMenu>> menus{};
	std::vector<Vector2D> mousePathOverlay{};
	DecorEditor decorEditor{};
	ContentEditor contentEditor{};
	RoomEditor roomEditor{};
	ItemEditor itemEditor{};
	MonsterEditor monsterEditor{};
	SpellEditor spellEditor{};

	MockGameContext()
	{
#ifdef TESTING_MODE
		dice.set_test_mode(true);
#endif
		try
		{
			tile_config.load(Paths::TILE_CONFIG);
		}
		catch (...)
		{
		}

		// Loaded loudly: a monster built without a body plan is a broken test,
		// not a test that quietly checks something else.
		body_plans.load(Paths::BODY_PLANS);

		// Armour class, missile and surprise rolls read the ability tables, as every
		// context the game builds can.
		data_manager.load_all_data(messages);

		// Casting, the spell menus, creating and spawning monsters and items read their
		// data through ctx, as the game's context does.
		spellRegistry.load(Paths::SPELLS);
		monsterRegistry.load(Paths::MONSTERS);
		itemRegistry.load(Paths::ITEMS);
		itemRegistry.load_enhanced_rules(Paths::ENHANCED_RULES);

		// Loaded as main.cpp loads it. An empty library lets generation pass in a fixture while
		// the game aborts on its first map, and a loader that opens nothing returns quietly.
		decorEditor.load_palette(Paths::TILE_CONFIG);
		prefabLibrary.load_tile_labels(Paths::TILE_CONFIG);
		prefabLibrary.load(Paths::PREFABS);
		assert(prefabLibrary.count() > 0 && "MockGameContext: no prefabs loaded");
	}

	GameContext to_game_context()
	{
		return GameContext{
			.map = &map,
			.playerOwner = &player,
			.playerBlueprint = &playerBlueprint,
			.messageSystem = &messages,
			.dice = &dice,
			.creatureManager = &creature_mgr,
			.levelManager = &levelManager,
			.stateManager = &stateManager,
			.menuManager = &menuManager,
			.displayManager = &displayManager,
			.gameLoopCoordinator = &gameLoopCoordinator,
			.dataManager = &data_manager,
			.targeting = &targeting,
			.hungerSystem = &hungerSystem,
			.buffSystem = &buffs,
			.floatingText = &floatingText,
			.contentRegistry = &content_registry,
			.costOverlay = &costOverlay,
			.pathfinder = &pathfinder,
			.decorEditor = &decorEditor,
			.prefabLibrary = &prefabLibrary,
			.contentEditor = &contentEditor,
			.roomEditor = &roomEditor,
			.itemEditor = &itemEditor,
			.monsterEditor = &monsterEditor,
			.spellEditor = &spellEditor,
			.tileConfig = &tile_config,
			.bodyPlanRegistry = &body_plans,
			.spellRegistry = &spellRegistry,
			.monsterRegistry = &monsterRegistry,
			.itemRegistry = &itemRegistry,
			.stairs = &stairs,
			.traps = &traps,
			.spellTiles = &spellTiles,
			.decorations = &decorations,
			.floorInventory = &inventory,
			.creatures = &creatures,
			.rooms = &rooms,
			.menus = &menus,
			.mousePathOverlay = &mousePathOverlay,
			.gameState = &game_state
		};
	}
};
