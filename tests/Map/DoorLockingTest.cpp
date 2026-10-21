// file: DoorLockingTest.cpp
// The one supported way to lock and unlock a door.
//
// Map had unlock_door and no counterpart. Locking lived in a bool parameter on
// set_door whose every caller passed a literal false, so the branch that locked
// anything was dead - and TreasureRoom locked its door by writing the tile's
// doorState directly, reaching past the map because there was nothing to call.
// It unlocked the same way, even though unlock_door already existed.
//
// So this pins the pair: lock_door and unlock_door are what move a door between
// CLOSED_UNLOCKED and CLOSED_LOCKED, each refusing anything that is not a closed
// door, and set_door places an unlocked one.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=DoorLockingTest.*

#include <gtest/gtest.h>

#include "src/Map.h"
#include "src/Vector2D.h"

namespace
{
constexpr int MAP_SIDE = 20;
constexpr Vector2D DOORWAY{ 5, 7 };
constexpr Vector2D PLAIN_FLOOR{ 8, 7 };
} // namespace

// set_door and dig_corridor are protected, so the map is reached through a type that
// exposes them rather than by widening the interface the game uses.
class MapWithADoorway : public Map
{
public:
	using Map::dig_corridor;
	using Map::Map;
	using Map::set_door;
};

class DoorLockingTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		map.init_tiles();
		map.set_tile(PLAIN_FLOOR, TileType::FLOOR);
		map.set_door(DOORWAY);
	}

	MapWithADoorway map{ MAP_SIDE, MAP_SIDE };
};

// A door is placed closed and unlocked; locking it is a separate act.
TEST_F(DoorLockingTest, ADoorIsPlacedUnlocked)
{
	EXPECT_TRUE(map.is_door(DOORWAY));
	EXPECT_FALSE(map.is_door_locked(DOORWAY));
}

TEST_F(DoorLockingTest, LockingADoorLocksIt)
{
	EXPECT_TRUE(map.lock_door(DOORWAY));

	EXPECT_TRUE(map.is_door_locked(DOORWAY));
}

// The pair is a round trip: what lock_door did, unlock_door undoes.
TEST_F(DoorLockingTest, UnlockingUndoesLocking)
{
	ASSERT_TRUE(map.lock_door(DOORWAY));

	EXPECT_TRUE(map.unlock_door(DOORWAY));
	EXPECT_FALSE(map.is_door_locked(DOORWAY));
}

// Each refuses what it cannot act on, and says so rather than doing nothing
// quietly: a door already locked, and a tile that is no door at all.
TEST_F(DoorLockingTest, LockingRefusesWhatItCannotLock)
{
	ASSERT_TRUE(map.lock_door(DOORWAY));

	EXPECT_FALSE(map.lock_door(DOORWAY)) << "a door already locked";
	EXPECT_FALSE(map.lock_door(PLAIN_FLOOR)) << "a stretch of floor is not a door";
	EXPECT_FALSE(map.is_door_locked(PLAIN_FLOOR));
}

// A door that resisted forcing stays locked, and is the only door that answers yes.
TEST_F(DoorLockingTest, ADoorThatResistedForcingStaysLocked)
{
	ASSERT_TRUE(map.lock_door(DOORWAY));
	EXPECT_FALSE(map.has_resisted_forcing(DOORWAY)) << "a door never tried";

	map.mark_resisted_forcing(DOORWAY);

	EXPECT_TRUE(map.has_resisted_forcing(DOORWAY));
	EXPECT_TRUE(map.is_door_locked(DOORWAY));
	EXPECT_FALSE(map.has_resisted_forcing(PLAIN_FLOOR));
}

// It is still locked, so locking it again is refused as for any locked door.
TEST_F(DoorLockingTest, LockingRefusesADoorThatResistedForcing)
{
	ASSERT_TRUE(map.lock_door(DOORWAY));
	map.mark_resisted_forcing(DOORWAY);

	EXPECT_FALSE(map.lock_door(DOORWAY));
	EXPECT_TRUE(map.has_resisted_forcing(DOORWAY)) << "the refused lock forgot the failed attempt";
}

// Table 1 limits forcing, not keys: unlocking it leaves an ordinary closed door.
TEST_F(DoorLockingTest, UnlockingOpensTheLockOfADoorThatResistedForcing)
{
	ASSERT_TRUE(map.lock_door(DOORWAY));
	map.mark_resisted_forcing(DOORWAY);

	EXPECT_TRUE(map.unlock_door(DOORWAY));
	EXPECT_FALSE(map.is_door_locked(DOORWAY));
	EXPECT_FALSE(map.has_resisted_forcing(DOORWAY));
}

// A lock remembers the level its pick failed at, and only a lock that was tried answers above 0.
TEST_F(DoorLockingTest, ALockRemembersTheLevelItsPickFailedAt)
{
	ASSERT_TRUE(map.lock_door(DOORWAY));
	EXPECT_EQ(map.lock_pick_failed_at_level(DOORWAY), 0) << "a lock never tried";

	map.mark_lock_pick_failed(DOORWAY, 3);

	EXPECT_EQ(map.lock_pick_failed_at_level(DOORWAY), 3);
	EXPECT_TRUE(map.is_door_locked(DOORWAY));
	EXPECT_EQ(map.lock_pick_failed_at_level(PLAIN_FLOOR), 0);
	EXPECT_EQ(map.lock_pick_failed_at_level(Vector2D{ -1, -1 }), 0) << "off the map";
}

// A door can have resisted forcing and a pick at once; neither mark hides the other.
TEST_F(DoorLockingTest, AFailedPickAndAFailedForcingAreBothKept)
{
	ASSERT_TRUE(map.lock_door(DOORWAY));
	map.mark_resisted_forcing(DOORWAY);
	map.mark_lock_pick_failed(DOORWAY, 1);

	EXPECT_TRUE(map.has_resisted_forcing(DOORWAY));
	EXPECT_EQ(map.lock_pick_failed_at_level(DOORWAY), 1);
}

// A corridor dug through a shut door leaves it shut: the tile stays a door, and the
// field-of-view grid, which is_wall reads, still calls it a wall. The row is the door's,
// so either order the corridor rolls crosses it.
TEST_F(DoorLockingTest, ACorridorThroughAShutDoorLeavesItShut)
{
	map.dig_corridor(Vector2D{ 2, DOORWAY.y }, Vector2D{ 9, DOORWAY.y });

	EXPECT_EQ(map.get_tile_type(DOORWAY), TileType::CLOSED_DOOR);
	EXPECT_TRUE(map.is_wall(DOORWAY)) << "the corridor opened the door in the grid alone";
}

// end of file: DoorLockingTest.cpp
