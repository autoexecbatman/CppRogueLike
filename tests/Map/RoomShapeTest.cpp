// file: RoomShapeTest.cpp
// A generated room keeps the shape it was given, its floor is one piece, and every
// corridor that reaches it opens onto that floor.
//
// What it is for. A room is drawn twice over: `RoomShape` carves part of its bounding
// box back to wall, and a prefab from `data/prefabs.json` then stamps its own walls into
// the same box. Neither sees the other, and both only ever turn floor into wall, so the
// pair can split a room in two or leave a whole edge with no floor for a corridor to
// enter by. Both layers are kept - they are what makes one room unlike the next - so
// `Map::create_room` carves, checks, and gives up a layer at a time when the result is
// unusable: shape over prefab, then prefab alone, then a plain box.
//
// **The prefab library must be loaded**, or `create_room` stamps nothing and this suite
// measures a generator no player runs. That is how this file passed while the real game
// aborted.
//
// A corridor tile inside a room's box is not that room's floor: a corridor crossing a
// corner of the box is walled off from the room, and counting it would call a sound room
// split.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RoomShapeTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <memory>
#include <ranges>
#include <string>
#include <vector>

#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/DecorEditor.h"
#include "src/Decoration.h"
#include "src/DungeonRoom.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/LevelManager.h"
#include "src/Map.h"
#include "src/Palette.h"
#include "src/Paths.h"
#include "src/Player.h"
#include "src/PrefabLibrary.h"
#include "src/Stairs.h"
#include "src/TileType.h"
#include "src/Trap.h"
#include "tests/mocks/MockGameContext.h"
#include "tests/mocks/TestDice.h"

namespace
{

constexpr int MAP_WIDTH = 120;
constexpr int MAP_HEIGHT = 70;

// How many dungeons a case builds. The shape defect appeared in about one room in a
// hundred, so a single map would have passed clean almost every time.
// One dungeon per seed, from 1 to DUNGEONS, so a failure names the seed that reproduces it.
constexpr unsigned int DUNGEONS = 40;

const char* shape_name(RoomShape shape)
{
	switch (shape)
	{
	case RoomShape::RECT:
	{
		return "RECT";
	}
	case RoomShape::L_SHAPE:
	{
		return "L_SHAPE";
	}
	case RoomShape::CHAMFERED:
	{
		return "CHAMFERED";
	}
	case RoomShape::CROSS:
	{
		return "CROSS";
	}
	case RoomShape::PILLARED:
	{
		return "PILLARED";
	}
	}
	return "unnamed shape";
}

// A map whose room carving a test can call on one room at a time.
class MapThatCarvesRooms : public Map
{
public:
	using Map::create_room;
	using Map::Map;
};

// Cells inside the room's own floor box that are wall. A shape carves these; a plain
// rectangle has none.
int walls_inside_box(const Map& map, const DungeonRoom& room)
{
	int walls = 0;
	for (int row = room.row; row <= room.row_end(); ++row)
	{
		for (int col = room.col; col <= room.col_end(); ++col)
		{
			if (map.get_tile_type(Vector2D{ col, row }) == TileType::WALL)
			{
				++walls;
			}
		}
	}
	return walls;
}

// Whether the given edge line of the room holds any floor. `alongRow` true reads the row
// `line` across the room's columns; false reads the column `line` down its rows.
bool edge_has_floor(const Map& map, const DungeonRoom& room, int line, bool alongRow)
{
	if (alongRow)
	{
		for (int column = room.col; column <= room.col_end(); ++column)
		{
			if (map.get_tile_type(Vector2D{ column, line }) != TileType::WALL)
			{
				return true;
			}
		}
		return false;
	}
	for (int row = room.row; row <= room.row_end(); ++row)
	{
		if (map.get_tile_type(Vector2D{ line, row }) != TileType::WALL)
		{
			return true;
		}
	}
	return false;
}

} // namespace

class RoomShapeTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// The generator needs real rolls; a queued-dice mock would build one fixed map.
		mock.dice.set_test_mode(false);

		// The game loads these in main.cpp, and create_room stamps a prefab only when
		// both are present: without them the generator takes a path no player ever sees.
		palette.load(Paths::TILE_CONFIG);
		prefabs.load(Paths::PREFABS);

		player = std::make_unique<Player>(Vector2D{ 1, 1 });
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->healthPool = std::make_unique<HealthPool>(20);
		player->set_strength(12);

		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		ctx.rooms = &rooms;
		ctx.stairs = &stairs;
		ctx.prefabLibrary = &prefabs;
		ctx.palette = &palette;
		ctx.decorEditor = &decorEditor;
		ctx.creatures = &creatures;
		ctx.levelManager = &levelManager;
		ctx.traps = &traps;
		ctx.decorations = &decorations;
		ctx.floorInventory = &floor;
	}

	// How many interior cells this prefab draws as wall, read from the library rather than
	// assumed: a prefab that draws none says nothing about whether its walls survived. The
	// border ring is skipped - it lines up with the room's own wall ring.
	int interior_wall_count_of(const std::string& prefabName) const
	{
		for (const Prefab& prefab : prefabs.all())
		{
			if (prefab.name != prefabName)
			{
				continue;
			}
			int walls = 0;
			for (size_t row = 1; row + 1 < prefab.rows.size(); ++row)
			{
				const std::string& line = prefab.rows[row];
				for (size_t column = 1; column + 1 < line.size(); ++column)
				{
					if (line[column] == '#')
					{
						++walls;
					}
				}
			}
			return walls;
		}
		return 0;
	}

	// Builds one dungeon into a fresh map, with everything the last one spawned cleared
	// first so one run cannot fill the next one's floor.
	void generate_into(Map& map, unsigned int seed)
	{
		mock.dice = RandomDice{ seed };
		ctx.map = &map;
		rooms.clear();
		creatures.clear();
		traps.clear();
		decorations.clear();
		floor.items.clear();
		map.init(ctx);
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> player{};
	LevelManager levelManager{};
	PrefabLibrary prefabs{};
	// Declared before the editor, which holds a reference to it.
	Palette palette{};
	DecorEditor decorEditor{ palette };
	std::vector<DungeonRoom> rooms{};
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::vector<std::unique_ptr<Trap>> traps{};
	std::vector<std::unique_ptr<Decoration>> decorations{};
	Stairs stairs{ Vector2D{ 1, 1 } };
	FloorInventory floor{};
};

TEST_F(RoomShapeTest, EveryRoomKeepsTheShapeItWasGiven)
{
	for (const unsigned int seed : std::views::iota(1u, DUNGEONS + 1))
	{
		Map map{ MAP_WIDTH, MAP_HEIGHT };
		generate_into(map, seed);

		for (const DungeonRoom& room : rooms)
		{
			// A prefab draws this room's interior itself, walls included, so the shape
			// says nothing about how many walls belong in the box. What must hold for
			// every room, prefab or not, is that its floor is one piece.
			if (!map.room_interior_is_one_piece(room))
			{
				std::string picture;
				for (int pictureRow = room.top_wall(); pictureRow <= room.bottom_wall(); ++pictureRow)
				{
					for (int pictureCol = room.left_wall(); pictureCol <= room.right_wall(); ++pictureCol)
					{
						const TileType type = map.get_tile_type(Vector2D{ pictureCol, pictureRow });
						picture += (type == TileType::WALL)	  ? '#'
							: (type == TileType::CLOSED_DOOR) ? '+'
							: (type == TileType::WATER)		  ? '~'
							: (type == TileType::CORRIDOR)	  ? ','
															  : '.';
					}
					picture += '\n';
				}
				FAIL() << shape_name(room.shape) << " at " << room.col << "," << room.row
					   << " " << room.width << "x" << room.height
					   << " prefab '" << room.prefabName << "' has floor split into pockets\n"
					   << picture;
			}
			if (!room.prefabName.empty())
			{
				continue;
			}

			const int walls = walls_inside_box(map, room);
			if (room.shape == RoomShape::RECT)
			{
				ASSERT_EQ(walls, 0)
					<< "a plain rectangle at " << room.col << "," << room.row
					<< " has " << walls << " walls inside it";
			}
			else
			{
				ASSERT_GT(walls, 0)
					<< shape_name(room.shape) << " at " << room.col << "," << room.row
					<< " " << room.width << "x" << room.height
					<< " was carved as a plain rectangle - something wrote over its shape";
			}
		}
	}
}

// A cross under each of these prefabs is unsound as carved: the first two wall off an edge,
// the last two split the floor. create_room drops the shape and keeps the prefab, so the
// room it leaves is sound and still holds the prefab's walls. A finished level cannot show
// this, because the steps after the room is carved reopen it.
TEST_F(RoomShapeTest, ACrossThePrefabBreaksIsCarvedAsThePrefabAlone)
{
	const std::vector<DungeonRoom> breakingRooms{
		DungeonRoom{ .col = 10, .row = 10, .width = 10, .height = 7, .shape = RoomShape::CROSS, .prefabName = "twin_chambers_12x9" },
		DungeonRoom{ .col = 10, .row = 10, .width = 10, .height = 7, .shape = RoomShape::CROSS, .prefabName = "guard_post_12x9" },
		DungeonRoom{ .col = 10, .row = 10, .width = 14, .height = 7, .shape = RoomShape::CROSS, .prefabName = "crypt_bays_16x9" },
		DungeonRoom{ .col = 10, .row = 10, .width = 10, .height = 7, .shape = RoomShape::CROSS, .prefabName = "moat_room_12x9" },
	};

	for (const DungeonRoom& room : breakingRooms)
	{
		ASSERT_GT(interior_wall_count_of(room.prefabName), 0) << "prefab '" << room.prefabName << "' is not in the library";
		MapThatCarvesRooms map{ MAP_WIDTH, MAP_HEIGHT };
		map.init_tiles();
		ctx.map = &map;
		rooms.clear();

		map.create_room(room, false, ctx);

		EXPECT_TRUE(map.room_layout_is_sound(room))
			<< "a cross under prefab '" << room.prefabName << "' was left split or with an edge no corridor can enter";
		EXPECT_GT(walls_inside_box(map, room), 0)
			<< "prefab '" << room.prefabName << "' was dropped along with the shape";
	}
}

// A prefab whose own wall cuts the room in two is unsound with or without a shape, so
// create_room gives up both layers and digs the plain box. No prefab in the library does
// this, so the test draws one.
TEST_F(RoomShapeTest, APrefabThatSplitsTheRoomByItselfLeavesAPlainBox)
{
	prefabs.add_or_replace(Prefab{
		.name = "wall_across_12x9",
		.rows = {
			"############",
			"#..........#",
			"#..........#",
			"#..........#",
			"############",
			"#..........#",
			"#..........#",
			"#..........#",
			"############",
		},
	});
	const DungeonRoom room{ .col = 10, .row = 10, .width = 10, .height = 7, .shape = RoomShape::CROSS, .prefabName = "wall_across_12x9" };
	MapThatCarvesRooms map{ MAP_WIDTH, MAP_HEIGHT };
	map.init_tiles();
	ctx.map = &map;

	map.create_room(room, false, ctx);

	EXPECT_TRUE(map.room_layout_is_sound(room)) << "the room was left split by its prefab's wall";
	EXPECT_EQ(walls_inside_box(map, room), 0) << "the plain box still holds walls";
}

TEST_F(RoomShapeTest, EveryCorridorOpensOntoTheRoomsFloor)
{
	for (const unsigned int seed : std::views::iota(1u, DUNGEONS + 1))
	{
		Map map{ MAP_WIDTH, MAP_HEIGHT };
		generate_into(map, seed);

		for (const DungeonRoom& room : rooms)
		{
			for (int neighbour : room.adjacentRoomIndices)
			{
				const DungeonRoom& other = rooms[static_cast<size_t>(neighbour)];

				// Map::place_from_graph aims the corridor at this room's centre column
				// or centre row, depending on which side the neighbour lies. This is the
				// first cell inside the room from where it arrives.
				Vector2D justInside{ -1, -1 };
				if (room.top_wall() > other.bottom_wall())
				{
					justInside = Vector2D{ map.exit_column(room, room.row), room.row };
				}
				else if (room.left_wall() > other.right_wall())
				{
					justInside = Vector2D{ room.col, map.exit_row(room, room.col) };
				}
				else if (room.bottom_wall() < other.top_wall())
				{
					justInside = Vector2D{ map.exit_column(room, room.row_end()), room.row_end() };
				}
				else if (room.right_wall() < other.left_wall())
				{
					justInside = Vector2D{ room.col_end(), map.exit_row(room, room.col_end()) };
				}
				else
				{
					continue;
				}

				ASSERT_NE(map.get_tile_type(justInside), TileType::WALL)
					<< "the corridor into " << shape_name(room.shape) << " at "
					<< room.col << "," << room.row << " opens onto rock at "
					<< justInside.x << "," << justInside.y;
			}
		}
	}
}

TEST_F(RoomShapeTest, RoomsNeverShareGround)
{
	// DungeonGenerator.h states it: each grid cell holds at most one room, so no two
	// rooms may share a cell of floor.
	for (const unsigned int seed : std::views::iota(1u, DUNGEONS + 1))
	{
		Map map{ MAP_WIDTH, MAP_HEIGHT };
		generate_into(map, seed);

		for (size_t first = 0; first < rooms.size(); ++first)
		{
			for (size_t second = first + 1; second < rooms.size(); ++second)
			{
				const DungeonRoom& one = rooms[first];
				const DungeonRoom& two = rooms[second];
				const bool overlap =
					one.col <= two.col_end() && two.col <= one.col_end() &&
					one.row <= two.row_end() && two.row <= one.row_end();
				ASSERT_FALSE(overlap)
					<< "rooms at " << one.col << "," << one.row << " and "
					<< two.col << "," << two.row << " share floor";
			}
		}
	}
}

TEST_F(RoomShapeTest, APrefabRoomKeepsTheWallsItsPrefabDraws)
{
	// A prefab that draws interior walls must still have them in the finished map. The
	// treasure vault used to clear its whole bounding box to floor, which erased them.
	for (const unsigned int seed : std::views::iota(1u, DUNGEONS + 1))
	{
		Map map{ MAP_WIDTH, MAP_HEIGHT };
		generate_into(map, seed);

		for (const DungeonRoom& room : rooms)
		{
			if (room.prefabName.empty())
			{
				continue;
			}
			if (interior_wall_count_of(room.prefabName) == 0)
			{
				continue;
			}
			EXPECT_GT(walls_inside_box(map, room), 0)
				<< "the room at " << room.col << "," << room.row
				<< " lost every wall its prefab '" << room.prefabName << "' draws";
		}
	}
}

// A prefab's decoration symbols are stamped as the palette's tiles for them: every cell where a
// prefab draws a decoration and the map kept an override holds that symbol's tile, and some do.
TEST_F(RoomShapeTest, APrefabsDecorationsAreThePalettesTiles)
{
	int stamped = 0;
	for (const unsigned int seed : std::views::iota(1u, DUNGEONS + 1))
	{
		Map map{ MAP_WIDTH, MAP_HEIGHT };
		generate_into(map, seed);

		for (const DungeonRoom& room : rooms)
		{
			for (const Prefab& prefab : prefabs.all())
			{
				if (prefab.name != room.prefabName)
				{
					continue;
				}
				for (size_t row = 0; row < prefab.rows.size(); ++row)
				{
					for (size_t column = 0; column < prefab.rows[row].size(); ++column)
					{
						const TileRef expected = palette.resolve_decor(prefab.rows[row][column]);
						const Vector2D cell{ room.left_wall() + static_cast<int>(column), room.top_wall() + static_cast<int>(row) };
						const TileRef placed = decorEditor.get_override(cell);
						if (!expected.is_valid() || !placed.is_valid())
						{
							continue;
						}
						EXPECT_EQ(placed, expected) << "prefab '" << prefab.name << "' symbol '" << prefab.rows[row][column] << "'";
						++stamped;
					}
				}
			}
		}
	}
	EXPECT_GT(stamped, 0) << "no prefab decoration was stamped in " << DUNGEONS << " dungeons";
}

// Every generated dungeon, saved and loaded into a fresh map, has the same tiles and the same
// locks - treasure rooms' locked doors included - and saves again to what it was loaded from.
TEST_F(RoomShapeTest, EveryDoorKeepsItsStateThroughASave)
{
	int lockedDoors = 0;
	for (const unsigned int seed : std::views::iota(1u, DUNGEONS + 1))
	{
		Map map{ MAP_WIDTH, MAP_HEIGHT };
		generate_into(map, seed);
		nlohmann::json saved;
		map.save(saved);

		Map fresh{ MAP_WIDTH, MAP_HEIGHT };
		fresh.load(saved);

		for (int row = 0; row < MAP_HEIGHT; ++row)
		{
			for (int col = 0; col < MAP_WIDTH; ++col)
			{
				const Vector2D cell{ col, row };
				EXPECT_EQ(fresh.get_tile_type(cell), map.get_tile_type(cell)) << "seed " << seed << " at " << col << "," << row;
				EXPECT_EQ(fresh.is_door_locked(cell), map.is_door_locked(cell)) << "seed " << seed << " at " << col << "," << row;
				lockedDoors += map.is_door_locked(cell) ? 1 : 0;
			}
		}
		nlohmann::json savedAgain;
		fresh.save(savedAgain);
		EXPECT_EQ(savedAgain, saved) << "seed " << seed;
	}
	EXPECT_GT(lockedDoors, 0) << "no treasure room locked a door in " << DUNGEONS << " dungeons, so no lock was saved";
}

TEST_F(RoomShapeTest, AnExitCellIsWalkableWheneverItsEdgeHasFloor)
{
	// The contract of exit_column and exit_row, stated without reference to how they
	// search: if the edge line holds any floor at all, the cell they name is floor.
	for (const unsigned int seed : std::views::iota(1u, DUNGEONS + 1))
	{
		Map map{ MAP_WIDTH, MAP_HEIGHT };
		generate_into(map, seed);

		for (const DungeonRoom& room : rooms)
		{
			if (edge_has_floor(map, room, room.row, true))
			{
				EXPECT_NE(map.get_tile_type(Vector2D{ map.exit_column(room, room.row), room.row }), TileType::WALL)
					<< "top edge of the room at " << room.col << "," << room.row;
			}
			if (edge_has_floor(map, room, room.row_end(), true))
			{
				EXPECT_NE(map.get_tile_type(Vector2D{ map.exit_column(room, room.row_end()), room.row_end() }), TileType::WALL)
					<< "bottom edge of the room at " << room.col << "," << room.row;
			}
			if (edge_has_floor(map, room, room.col, false))
			{
				EXPECT_NE(map.get_tile_type(Vector2D{ room.col, map.exit_row(room, room.col) }), TileType::WALL)
					<< "left edge of the room at " << room.col << "," << room.row;
			}
			if (edge_has_floor(map, room, room.col_end(), false))
			{
				EXPECT_NE(map.get_tile_type(Vector2D{ room.col_end(), map.exit_row(room, room.col_end()) }), TileType::WALL)
					<< "right edge of the room at " << room.col << "," << room.row;
			}
		}
	}
}

TEST_F(RoomShapeTest, AnExitCellStepsAsideWhenTheCentreIsWalled)
{
	// The situation exit_column exists for, built rather than waited for: a room whose
	// centre column is wall on the edge a corridor would leave by. Generation reaches it
	// only now and then, so a case that builds it is the one that can fail on demand.
	Map map{ MAP_WIDTH, MAP_HEIGHT };
	map.init_tiles();

	const DungeonRoom room{ 10, 10, 9, 5 };
	for (int row = room.row; row <= room.row_end(); ++row)
	{
		for (int column = room.col; column <= room.col_end(); ++column)
		{
			map.set_tile(Vector2D{ column, row }, TileType::FLOOR);
		}
	}
	map.set_tile(Vector2D{ room.center_col(), room.row }, TileType::WALL);

	const int chosen = map.exit_column(room, room.row);
	EXPECT_NE(chosen, room.center_col()) << "the exit stayed on the walled centre column";
	EXPECT_EQ(map.get_tile_type(Vector2D{ chosen, room.row }), TileType::FLOOR)
		<< "the exit column " << chosen << " is not floor";
}
