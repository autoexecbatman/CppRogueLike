// file: TileDefinitionTest.cpp
// Verifies that every tile type the game can place has a definition, and that
// the shipped data says what the code it replaced used to say.
//
// Tile behaviour was six switches in five files. Five carried a default case,
// so adding a TileType compiled clean and the new tile silently behaved like
// whatever the default said. There is now one table, and a type missing from it
// throws on lookup rather than defaulting.

#include <gtest/gtest.h>

#include "src/Actor/Actor.h"
#include "src/Map/TileType.h"
#include "src/Systems/TileConfig.h"

class TileDefinitionTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		config.load("data/tiles/tile_config.json");
	}

	TileConfig config{};
};

// Every type the enum can hold must be authored. A missing one throws, which is
// the replacement for five silent default cases.
TEST_F(TileDefinitionTest, EveryTileTypeHasADefinition)
{
	for (const auto tileType : { TileType::FLOOR, TileType::WALL, TileType::WATER,
			 TileType::CLOSED_DOOR, TileType::OPEN_DOOR, TileType::CORRIDOR })
	{
		EXPECT_NO_THROW((void)config.get_tile_definition(tileType))
			<< "tile type " << static_cast<int>(tileType) << " has no entry in tile_config.json";
	}
}

// Walls stop everything and nothing crosses them.
TEST_F(TileDefinitionTest, WallBlocksWithNoBypass)
{
	const TileDefinition& wall = config.get_tile_definition(TileType::WALL);

	EXPECT_TRUE(wall.blocksMovement);
	EXPECT_FALSE(wall.hasBypassState);
}

// Water blocks, but swimming carries a creature through.
TEST_F(TileDefinitionTest, WaterBlocksUnlessTheCreatureCanSwim)
{
	const TileDefinition& water = config.get_tile_definition(TileType::WATER);

	EXPECT_TRUE(water.blocksMovement);
	ASSERT_TRUE(water.hasBypassState);
	EXPECT_EQ(water.bypassState, ActorState::CAN_SWIM);
}

// Open ground is passable and unremarkable.
TEST_F(TileDefinitionTest, OpenGroundIsPassableAndSilent)
{
	for (const auto tileType : { TileType::FLOOR, TileType::CORRIDOR, TileType::OPEN_DOOR })
	{
		const TileDefinition& definition = config.get_tile_definition(tileType);

		EXPECT_FALSE(definition.blocksMovement);
		EXPECT_TRUE(definition.entryMessage.empty()) << definition.displayName << " announces itself";
	}
}

// Every tile is nameable, since the hover tooltip reads this.
TEST_F(TileDefinitionTest, EveryTileHasADisplayName)
{
	for (const auto tileType : { TileType::FLOOR, TileType::WALL, TileType::WATER,
			 TileType::CLOSED_DOOR, TileType::OPEN_DOOR, TileType::CORRIDOR })
	{
		const TileDefinition& definition = config.get_tile_definition(tileType);

		EXPECT_FALSE(definition.displayName.empty());
		EXPECT_NE(definition.displayName, "Unknown") << "left at the struct default";
	}
}

// A minimap colour must be authored, or the tile draws as transparent black.
TEST_F(TileDefinitionTest, EveryTileHasMinimapColours)
{
	for (const auto tileType : { TileType::FLOOR, TileType::WALL, TileType::WATER,
			 TileType::CLOSED_DOOR, TileType::OPEN_DOOR, TileType::CORRIDOR })
	{
		const TileDefinition& definition = config.get_tile_definition(tileType);

		EXPECT_GT(definition.minimapVisible.alpha, 0) << definition.displayName << " is invisible in view";
		EXPECT_GT(definition.minimapRemembered.alpha, 0) << definition.displayName << " is invisible from memory";
	}
}
