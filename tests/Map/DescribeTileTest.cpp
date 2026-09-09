// file: DescribeTileTest.cpp
// Verifies which tiles Map::describe_tile remarks on, and that it stays silent
// on the rest.
//
// It was tile_action, took a Creature it never read, and had five branches of
// which four could not run: wall and closed door are collisions, so a move onto
// one never completes and the player never stands on one, while floor and
// corridor had empty bodies. Those two impossible cases are now asserts rather
// than dead message branches.

#include <gtest/gtest.h>

#include "src/Map/Map.h"
#include "tests/mocks/MockGameContext.h"

class DescribeTileTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		map.init_tiles();
	}

	// The message system keeps only the latest message, so a silent call leaves
	// whatever was there before.
	std::string message_after(TileType tileType)
	{
		mock.messages.message(WHITE_BLACK_PAIR, "sentinel", true);
		map.describe_tile(tileType, ctx);
		return mock.messages.get_current_message();
	}

	MockGameContext mock{};
	GameContext ctx{};
	Map map{ 20, 20 };
};

// Water is the one tile worth remarking on.
TEST_F(DescribeTileTest, WaterIsAnnounced)
{
	EXPECT_NE(message_after(TileType::WATER).find("water"), std::string::npos);
}

// Ordinary ground says nothing, so walking does not spam the log.
TEST_F(DescribeTileTest, OrdinaryGroundIsSilent)
{
	EXPECT_EQ(message_after(TileType::FLOOR), "sentinel");
	EXPECT_EQ(message_after(TileType::CORRIDOR), "sentinel");
	EXPECT_EQ(message_after(TileType::OPEN_DOOR), "sentinel");
}
