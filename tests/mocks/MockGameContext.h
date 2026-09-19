#pragma once
#include "src/InventoryData.h"
#include "src/GameContext.h"
#include "src/Paths.h"
#include "src/RandomDice.h"
#include "src/ContentRegistry.h"
#include "src/CreatureManager.h"
#include "src/DataManager.h"
#include "src/MessageSystem.h"
#include "src/BodyPlanRegistry.h"
#include "src/SpellRegistry.h"
#include "src/TileConfig.h"

struct MockGameContext
{
	RandomDice dice{};
	MessageSystem messages{};
	CreatureManager creature_mgr{};
	DataManager data_manager{};
	ContentRegistry content_registry{};
	FloorInventory inventory{ 100 };
	GameState game_state{};
	TileConfig tile_config{};
	BodyPlanRegistry body_plans{};
	SpellRegistry spellRegistry{};

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

		// Casting and the spell menus read spells through ctx, as the game's context does.
		spellRegistry.load(Paths::SPELLS);
	}

	GameContext to_game_context()
	{
		return GameContext{
			.messageSystem = &messages,
			.dice = &dice,
			.creatureManager = &creature_mgr,
			.dataManager = &data_manager,
			.contentRegistry = &content_registry,
			.tileConfig = &tile_config,
			.bodyPlanRegistry = &body_plans,
			.spellRegistry = &spellRegistry,
			.floorInventory = &inventory,
			.gameState = &game_state
		};
	}
};
