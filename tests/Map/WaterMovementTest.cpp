// file: WaterMovementTest.cpp
// Verifies that water bars a creature that cannot swim and admits one that can.
//
// This asks the predicate directly. MonsterWaterAvoidanceTest asks the same rule
// through a monster's turn, which is the half that was missing: the rule was correct
// here for a year while every monster walked into pools, because the gate the AI used
// was a different function that took no creature.

#include <gtest/gtest.h>

#include "src/Colors.h"
#include "src/Creature.h"
#include "src/Player.h"
#include "src/Map.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

class WaterMovementTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.creatures = &creatures;
		ctx.playerOwner = &player;

		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);

		map.init_tiles();
		map.set_tile(waterTile, TileType::WATER, 10.0);
		map.set_tile(floorTile, TileType::FLOOR, 1.0);
	}

	MockGameContext mock{};
	GameContext ctx{};
	Map map{ 20, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 1, 1 }) };
	std::vector<std::unique_ptr<Creature>> creatures{};
	Creature swimmer{ Vector2D{ 2, 2 }, ActorData{ TileRef{}, "spider", ColorPairId::WHITE_BLACK } };
	Vector2D waterTile{ 5, 5 };
	Vector2D floorTile{ 6, 5 };
};

// A creature with no swim ability cannot enter water.
TEST_F(WaterMovementTest, WaterBlocksACreatureThatCannotSwim)
{
	EXPECT_FALSE(map.can_walk(*player, waterTile, ctx));
}

// CAN_SWIM is what lets a creature cross; spiders set it at construction.
TEST_F(WaterMovementTest, WaterAdmitsACreatureThatCanSwim)
{
	swimmer.add_state(ActorState::CAN_SWIM);

	EXPECT_TRUE(map.can_walk(swimmer, waterTile, ctx));
}

// Dry land is unaffected either way, so the rule is about water alone.
TEST_F(WaterMovementTest, FloorIsPassableRegardlessOfSwimming)
{
	EXPECT_TRUE(map.can_walk(*player, floorTile, ctx));

	player->add_state(ActorState::CAN_SWIM);

	EXPECT_TRUE(map.can_walk(*player, floorTile, ctx));
}

// Losing the ability closes the water again.
TEST_F(WaterMovementTest, RemovingSwimClosesTheWater)
{
	player->add_state(ActorState::CAN_SWIM);
	ASSERT_TRUE(map.can_walk(*player, waterTile, ctx));

	player->remove_state(ActorState::CAN_SWIM);

	EXPECT_FALSE(map.can_walk(*player, waterTile, ctx));
}
