#pragma once
#include "src/InventoryData.h"
#include "src/GameContext.h"
#include "src/Paths.h"
#include "src/RandomDice.h"
#include "src/ContentRegistry.h"
#include "src/CreatureManager.h"
#include "src/MessageSystem.h"
#include "src/BodyPlanRegistry.h"
#include "src/TileConfig.h"

struct MockGameContext
{
	RandomDice dice{};
	MessageSystem messages{};
	CreatureManager creature_mgr{};
	ContentRegistry content_registry{};
	FloorInventory inventory{ 100 };
	GameState game_state{};
	TileConfig tile_config{};
	BodyPlanRegistry body_plans{};

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
	}

	GameContext to_game_context()
	{
		return GameContext{
			.messageSystem = &messages,
			.dice = &dice,
			.creatureManager = &creature_mgr,
			.contentRegistry = &content_registry,
			.tileConfig = &tile_config,
			.bodyPlanRegistry = &body_plans,
			.floorInventory = &inventory,
			.gameState = &game_state
		};
	}
};
