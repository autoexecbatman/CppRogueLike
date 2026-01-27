#pragma once
#include "src/Random/RandomDice.h"
#include "src/Systems/MessageSystem.h"
#include "src/Systems/CreatureManager.h"
#include "src/Actor/InventoryData.h"
#include "src/Core/GameContext.h"

struct MockGameContext
{
    RandomDice dice;
    MessageSystem messages;
    CreatureManager creature_mgr;
    InventoryData inventory{100};  // Initialize with capacity of 100
    GameStatus status = GameStatus::IDLE;

    MockGameContext()
    {
#ifdef TESTING_MODE
        dice.set_test_mode(true);
#endif
    }

    GameContext to_game_context()
    {
        GameContext ctx{};
        ctx.dice = &dice;
        ctx.message_system = &messages;
        ctx.creature_manager = &creature_mgr;
        ctx.inventory_data = &inventory;
        ctx.game_status = &status;
        // display_manager is intentionally nullptr for headless tests
        return ctx;
    }
};
