#pragma once
#include "src/Actor/InventoryData.h"
#include "src/Core/GameContext.h"
#include "src/Core/Paths.h"
#include "src/Random/RandomDice.h"
#include "src/Systems/ContentRegistry.h"
#include "src/Systems/CreatureManager.h"
#include "src/Systems/MessageSystem.h"
#include "src/Systems/BodyPlanRegistry.h"
#include "src/Systems/TileConfig.h"

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
