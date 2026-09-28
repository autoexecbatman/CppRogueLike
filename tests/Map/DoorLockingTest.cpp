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

// set_door is protected, so the map is reached through a type that exposes it
// rather than by widening the interface the game uses.
class MapWithADoorway : public Map
{
public:
	using Map::Map;
	using Map::set_door;
};

class DoorLockingTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		map.init_tiles();
		map.set_tile(PLAIN_FLOOR, TileType::FLOOR, 1.0);
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

// end of file: DoorLockingTest.cpp
