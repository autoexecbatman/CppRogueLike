// file: RoomDoorsTest.cpp
// A key opens a locked room: Map::open_all_room_doors finds the room the used door
// belongs to and opens every locked door on that room's wall ring. The doors expected
// to open are worked out from the rooms' walls below, never from what the function
// returns.
//
// Two rooms share a wall column, so one door belongs to both. Opening from the first
// room opens its own doors and the shared one, and leaves the second room's far door
// shut. A door that belongs to no room opens alone.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RoomDoorsTest.*

#include <gtest/gtest.h>

#include <vector>

#include "src/DungeonRoom.h"
#include "src/GameContext.h"
#include "src/Map.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr int MAP_WIDTH = 30;
constexpr int MAP_HEIGHT = 20;

// Room A's floor is columns 2-6 and rows 2-5, so its walls are column 1 and 7 and rows
// 1 and 6. Room B's floor is columns 8-11 on the same rows, so its left wall is column
// 7 as well: the two rooms share that column.
const DungeonRoom ROOM_A{ .col = 2, .row = 2, .width = 5, .height = 4 };
const DungeonRoom ROOM_B{ .col = 8, .row = 2, .width = 4, .height = 4 };

// On room A's top wall, above floor (4, 2).
constexpr Vector2D A_NORTH_DOOR{ 4, 1 };
// On room A's bottom wall, below floor (3, 5).
constexpr Vector2D A_SOUTH_DOOR{ 3, 6 };
// In the shared column, between A's floor (6, 3) and B's floor (8, 3).
constexpr Vector2D SHARED_DOOR{ 7, 3 };
// On room B's right wall (column 12), beside floor (11, 4).
constexpr Vector2D B_EAST_DOOR{ 12, 4 };
// Far from both rooms, so it belongs to neither.
constexpr Vector2D LONE_DOOR{ 20, 15 };
} // namespace

// set_door is protected, so the map is reached through a type that exposes it.
class MapWithRoomDoors : public Map
{
public:
	using Map::Map;
	using Map::set_door;
};

class RoomDoorsTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		map.init_tiles();
		for (const DungeonRoom& room : { ROOM_A, ROOM_B })
		{
			for (int row = room.row; row <= room.row_end(); ++row)
			{
				for (int col = room.col; col <= room.col_end(); ++col)
				{
					map.set_tile(Vector2D{ col, row }, TileType::FLOOR);
				}
			}
		}
		for (
			const Vector2D door :
			{ A_NORTH_DOOR, A_SOUTH_DOOR, SHARED_DOOR, B_EAST_DOOR, LONE_DOOR })
		{
			map.set_door(door);
			ASSERT_TRUE(map.lock_door(door));
		}

		rooms = { ROOM_A, ROOM_B };
		ctx = mock.to_game_context();
		ctx.map = &map;
		ctx.rooms = &rooms;
	}

	MockGameContext mock{};
	GameContext ctx{};
	MapWithRoomDoors map{ MAP_WIDTH, MAP_HEIGHT };
	std::vector<DungeonRoom> rooms{};
};

TEST_F(RoomDoorsTest, AKeyOpensEveryLockedDoorOfItsRoom)
{
	map.open_all_room_doors(A_NORTH_DOOR, ctx);

	EXPECT_TRUE(map.is_open_door(A_NORTH_DOOR))
		<< "the door the key was used on";
	EXPECT_TRUE(map.is_open_door(A_SOUTH_DOOR))
		<< "another door of the same room";
	EXPECT_TRUE(map.is_open_door(SHARED_DOOR))
		<< "a door on the room's shared wall";
}

TEST_F(RoomDoorsTest, AnotherRoomsDoorStaysLocked)
{
	map.open_all_room_doors(A_NORTH_DOOR, ctx);

	EXPECT_TRUE(map.is_door_locked(B_EAST_DOOR))
		<< "the second room's far door opened";
	EXPECT_TRUE(map.is_door_locked(LONE_DOOR)) << "a door of no room opened";
}

TEST_F(RoomDoorsTest, ADoorOfNoRoomOpensAlone)
{
	map.open_all_room_doors(LONE_DOOR, ctx);

	EXPECT_TRUE(map.is_open_door(LONE_DOOR));
	for (
		const Vector2D door :
		{ A_NORTH_DOOR, A_SOUTH_DOOR, SHARED_DOOR, B_EAST_DOOR })
	{
		EXPECT_TRUE(map.is_door_locked(door)) << door.x << "," << door.y;
	}
}

// end of file: RoomDoorsTest.cpp
