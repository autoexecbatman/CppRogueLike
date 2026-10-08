// file: WaterSpawnTest.cpp
//
// That nothing is placed standing in water.
//
// What it is for. is_water is consulted in four places in the whole source, and three
// of them place items and stairs. No creature placement path asked it, and can_walk -
// the predicate every monster spawn goes through - tests wall, closed door, actor and
// decoration and never water. can_walk takes no creature either, so it could not ask
// whether this one swims.
//
// The ordering is why: find_spawn_position picks the tile, then add_monster picks what
// stands there, so the tile is committed before anyone knows what will occupy it. Of 38
// monsters, 35 cannot swim, and one placed in water holds its breath for a round or two
// and then rolls to drown.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=WaterSpawnTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/Creature.h"
#include "src/DungeonRoom.h"
#include "src/Map.h"
#include "src/Player.h"
#include "src/SpawnUtils.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// A room small enough that a water-filled one leaves only a couple of dry tiles, so a
// picker that ignores water lands on one almost every draw.
constexpr int ROOM_COL = 2;
constexpr int ROOM_ROW = 2;
constexpr int ROOM_SIZE = 4;
constexpr int DRAWS = 200;
} // namespace

class WaterSpawnTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		ctx.creatures = &creatures;
		ctx.map = &map;

		map.init_tiles();
		for (int column = ROOM_COL; column < ROOM_COL + ROOM_SIZE; ++column)
		{
			for (int row = ROOM_ROW; row < ROOM_ROW + ROOM_SIZE; ++row)
			{
				map.set_tile(Vector2D{ column, row }, TileType::FLOOR, 1.0);
			}
		}
	}

	// Floods every tile of the room but the one named, so a picker that ignores water
	// has fifteen chances in sixteen of answering wrongly.
	void flood_all_but(Vector2D dryTile)
	{
		for (int column = ROOM_COL; column < ROOM_COL + ROOM_SIZE; ++column)
		{
			for (int row = ROOM_ROW; row < ROOM_ROW + ROOM_SIZE; ++row)
			{
				const Vector2D here{ column, row };
				if (!(here == dryTile))
				{
					map.set_tile(here, TileType::WATER, 1.0);
				}
			}
		}
	}

	MockGameContext mock{};
	GameContext ctx{};
	Map map{ 20, 20 };
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::unique_ptr<Player> player{};
	DungeonRoom room{ ROOM_COL, ROOM_ROW, ROOM_SIZE, ROOM_SIZE };
};

TEST_F(WaterSpawnTest, ADryRoomStillAnswers)
{
	// The other half: the exclusion must not refuse every tile it is offered.
	const auto position = SpawnUtils::find_random_room_position(room, ctx);

	ASSERT_TRUE(position.has_value()) << "a room of dry floor gave no spawn tile at all";
	EXPECT_FALSE(map.is_water(*position));
}

TEST_F(WaterSpawnTest, NoSpawnTileIsEverWater)
{
	const Vector2D onlyDryTile{ ROOM_COL, ROOM_ROW };
	flood_all_but(onlyDryTile);

	for (int draw = 0; draw < DRAWS; ++draw)
	{
		const auto position = SpawnUtils::find_random_room_position(room, ctx);
		ASSERT_TRUE(position.has_value()) << "the one dry tile was never offered";
		ASSERT_FALSE(map.is_water(*position))
			<< "draw " << draw << " put a creature in the water at "
			<< position->x << "," << position->y;
	}
}
