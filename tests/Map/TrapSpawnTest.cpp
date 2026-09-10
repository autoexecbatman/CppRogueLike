// Checks that a spawned trap lands in the trap container and nowhere else.
//
// What it is for. Traps and spell tiles were split out of one shared vector, and the
// wiring that carries a new trap from Map::spawn_traps into ctx.traps is the part of that
// split nothing exercised. A trap pushed into the wrong container, or into none, would
// leave the dungeon quietly trapless: the game still runs, the suite still passes, and the
// only symptom is a floor that feels empty. That is indistinguishable from bad luck when
// three rooms in ten carry traps anyway.
//
// What it deliberately does not check. Not where traps are placed within a room, not how
// often, and not what they do on entry. Only that a spawned one is reachable afterwards
// through the container the rest of the game reads.
//
// The rolls are forced rather than seeded. RandomDice's test mode serves a queue in call
// order, so the sequence below is the exact sequence spawn_traps makes: the room check,
// the trap count, a column, a row, and the trap type.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=TrapSpawnTest.*

#include "src/Map/Map.h"
#include "src/Map/DungeonRoom.h"
#include "src/Objects/SpellTile.h"
#include "src/Objects/Trap.h"
#include "tests/mocks/MockGameContext.h"
#include <gtest/gtest.h>

#include <memory>
#include <vector>

// spawn_traps is protected: it is an implementation detail of level generation, not
// something the game calls from outside. A derived class is how that is reached without
// widening the real interface for a test's convenience.
class TestableMap : public Map
{
public:
	using Map::Map;
	using Map::spawn_traps;
};

class TrapSpawnTest : public ::testing::Test
{
protected:
	MockGameContext mock;
	std::unique_ptr<TestableMap> map;
	std::vector<std::unique_ptr<Trap>> traps;
	std::vector<std::unique_ptr<SpellTile>> spellTiles;
	GameContext ctx;
	DungeonRoom room{};

	// A room of floor at a known place, big enough that a trap has somewhere to go.
	static constexpr int ROOM_COL = 5;
	static constexpr int ROOM_ROW = 5;
	static constexpr int ROOM_SIZE = 6;

	void SetUp() override
	{
		map = std::make_unique<TestableMap>(40, 30);
		map->init_tiles();

		ctx = mock.to_game_context();
		ctx.map = map.get();
		ctx.traps = &traps;
		ctx.spellTiles = &spellTiles;

		room.col = ROOM_COL;
		room.row = ROOM_ROW;
		room.width = ROOM_SIZE;
		room.height = ROOM_SIZE;

		for (int row = room.row; row <= room.row_end(); ++row)
		{
			for (int col = room.col; col <= room.col_end(); ++col)
			{
				map->set_tile(Vector2D{ col, row }, TileType::FLOOR, 1.0);
			}
		}

		mock.dice.set_test_mode(true);
	}

	void TearDown() override
	{
		mock.dice.set_test_mode(false);
		mock.dice.clear_fixed_rolls();
	}
};

// The room check passes, one trap is asked for, and it lands in ctx.traps.
TEST_F(TrapSpawnTest, ASpawnedTrapLandsInTheTrapContainer)
{
	// d10 <= 3 so the room gets traps; then one trap, at room offset 2,2, of type PIT.
	for (int roll : { 1, 1, 2, 2, 1 })
	{
		mock.dice.set_next_roll(roll);
	}

	map->spawn_traps(room, ctx);

	ASSERT_EQ(traps.size(), 1u) << "the trap never reached the container the game reads";
	EXPECT_EQ(traps[0]->position.x, ROOM_COL + 2);
	EXPECT_EQ(traps[0]->position.y, ROOM_ROW + 2);
	EXPECT_EQ(traps[0]->get_type(), TrapType::PIT);
}

// A trap is not a spell tile. Nothing may leak into the other container.
TEST_F(TrapSpawnTest, SpawningATrapLeavesTheSpellTilesAlone)
{
	for (int roll : { 1, 1, 2, 2, 1 })
	{
		mock.dice.set_next_roll(roll);
	}

	map->spawn_traps(room, ctx);

	EXPECT_TRUE(spellTiles.empty()) << "a trap was put in the spell tile container";
}

// Seven rooms in ten carry no traps at all, and that path must leave the container empty
// rather than pushing something inert.
TEST_F(TrapSpawnTest, ARoomThatRollsNoTrapsGetsNone)
{
	mock.dice.set_next_roll(10); // d10 above 3: this room has no traps

	map->spawn_traps(room, ctx);

	EXPECT_TRUE(traps.empty());
	EXPECT_TRUE(spellTiles.empty());
}
