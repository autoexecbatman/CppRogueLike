// file: FullDungeonSpawnTest.cpp
//
// What happens when there is nowhere left to put a monster.
//
// What it is for. Spawning asked two questions by random search and had no answer for
// "there is no free tile". find_random_room_position drew tiles until one passed, and
// CreatureManager::find_spawn_position drew rooms until one answered - both in a
// while (true) with no counter. A dungeon with no room for anything did not return a
// bad position; it never returned at all, and the game stopped.
//
// Both questions are answered exactly now: a room's free tiles are enumerated, and
// every room is visited at most once. No space is a legal outcome, and the round
// simply passes without a monster.
//
// Tried and rejected as a mutation anchor: the roomCount == 0 fallthrough in
// find_spawn_position. An assert fires on that case first, so in Debug the branch is
// unreachable and no test can distinguish it; it exists so the Release build, where the
// assert is compiled out, returns nothing rather than reading past the end.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=FullDungeonSpawnTest.*

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "src/Colors.h"
#include "src/CreatureManager.h"
#include "src/DungeonRoom.h"
#include "src/LevelManager.h"
#include "src/Map.h"
#include "src/Player.h"
#include "src/SpawnUtils.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr int ROOM_COL = 2;
constexpr int ROOM_ROW = 2;
constexpr int ROOM_SIZE = 3;
} // namespace

class FullDungeonSpawnTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		ctx.creatures = &creatures;
		ctx.map = &map;
		ctx.rooms = &rooms;
		ctx.creatureManager = &manager;
		// add_monster only builds anything when a levelManager is wired, so without this
		// the spawn is inert and a test of "nothing was spawned" passes for the wrong
		// reason. A mutation that spawned on the empty path survived until this was here.
		ctx.levelManager = &levels;

		map.init_tiles();
		carve(TileType::FLOOR);
		rooms.push_back(DungeonRoom{ ROOM_COL, ROOM_ROW, ROOM_SIZE, ROOM_SIZE });
	}

	void carve(TileType type)
	{
		for (int column = ROOM_COL; column < ROOM_COL + ROOM_SIZE; ++column)
		{
			for (int row = ROOM_ROW; row < ROOM_ROW + ROOM_SIZE; ++row)
			{
				map.set_tile(Vector2D{ column, row }, type, 1.0);
			}
		}
	}

	MockGameContext mock{};
	GameContext ctx{};
	Map map{ 20, 20 };
	CreatureManager manager{};
	LevelManager levels{};
	std::vector<DungeonRoom> rooms{};
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::unique_ptr<Player> player{};
};

TEST_F(FullDungeonSpawnTest, ARoomOfWaterOffersNoTile)
{
	carve(TileType::WATER);

	EXPECT_FALSE(SpawnUtils::find_random_room_position(rooms.front(), ctx).has_value())
		<< "a room with nothing but water offered a tile to stand on";
}

TEST_F(FullDungeonSpawnTest, AFullDungeonSpawnsNothingAndReturns)
{
	// The whole point: this must come back. Before the fix it never did.
	carve(TileType::WATER);
	const size_t before = creatures.size();

	manager.spawn_creatures(ctx);

	EXPECT_EQ(creatures.size(), before) << "a monster was put somewhere there was no room";
}

TEST_F(FullDungeonSpawnTest, ARoomWithSpaceDoesSpawn)
{
	// The guard in spawn_creatures has to be readable both ways: with somewhere to put a
	// monster, one appears. Without this the fix could be "never spawn" and pass.
	const size_t before = creatures.size();

	manager.spawn_creatures(ctx);

	EXPECT_GT(creatures.size(), before) << "a dry room with space produced no monster";
}

TEST_F(FullDungeonSpawnTest, ADryRoomStillSpawns)
{
	// The other half: the give-up path must not swallow the ordinary case.
	const auto position = SpawnUtils::find_random_room_position(rooms.front(), ctx);

	ASSERT_TRUE(position.has_value()) << "a room of dry floor offered no tile at all";
	EXPECT_FALSE(map.is_water(*position));
}
