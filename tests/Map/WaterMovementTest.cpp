// file: WaterMovementTest.cpp
// Verifies that water bars a creature that cannot swim and admits one that can.
//
// The rule was present in Map::is_collision as a commented-out line, replaced
// by "return false" - so water blocked nobody, the "You can't swim." message in
// PlayerController was unreachable, and ActorState::CAN_SWIM was authored in
// the monster editor and read by nothing that affects movement.

#include <gtest/gtest.h>

#include "src/Actor/Creature.h"
#include "src/ActorTypes/Player.h"
#include "src/Map/Map.h"
#include "src/Utils/Vector2D.h"
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
	Creature swimmer{ Vector2D{ 2, 2 }, ActorData{ TileRef{}, "spider", 0 } };
	Vector2D waterTile{ 5, 5 };
	Vector2D floorTile{ 6, 5 };
};

// A creature with no swim ability cannot enter water.
TEST_F(WaterMovementTest, WaterBlocksACreatureThatCannotSwim)
{
	EXPECT_TRUE(map.is_collision(*player, TileType::WATER, waterTile, ctx));
}

// CAN_SWIM is what lets a creature cross; spiders set it at construction.
TEST_F(WaterMovementTest, WaterAdmitsACreatureThatCanSwim)
{
	swimmer.add_state(ActorState::CAN_SWIM);

	EXPECT_FALSE(map.is_collision(swimmer, TileType::WATER, waterTile, ctx));
}

// Dry land is unaffected either way, so the rule is about water alone.
TEST_F(WaterMovementTest, FloorIsPassableRegardlessOfSwimming)
{
	EXPECT_FALSE(map.is_collision(*player, TileType::FLOOR, floorTile, ctx));

	player->add_state(ActorState::CAN_SWIM);

	EXPECT_FALSE(map.is_collision(*player, TileType::FLOOR, floorTile, ctx));
}

// Losing the ability closes the water again.
TEST_F(WaterMovementTest, RemovingSwimClosesTheWater)
{
	player->add_state(ActorState::CAN_SWIM);
	ASSERT_FALSE(map.is_collision(*player, TileType::WATER, waterTile, ctx));

	player->remove_state(ActorState::CAN_SWIM);

	EXPECT_TRUE(map.is_collision(*player, TileType::WATER, waterTile, ctx));
}
