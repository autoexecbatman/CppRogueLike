// file: MapDoorPersistenceTest.cpp
//
// A door keeps its state through a save and a load: a locked door is still locked, a closed
// one still closed and unlocked, an open one still open, and a map saved again after loading
// writes what it was loaded from. The save used to hold only each tile's type, so every lock -
// a treasure room's included - was gone after a load (issues/0303).
//
// Generated dungeons, with their treasure rooms locked by TreasureRoom, are checked the same way
// in RoomShapeTest.EveryDoorKeepsItsStateThroughASave.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=MapDoorPersistenceTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <format>
#include <optional>
#include <stdexcept>
#include <string>

#include "src/Map.h"
#include "src/TileType.h"
#include "src/Vector2D.h"

namespace
{
constexpr int DOOR_MAP_WIDTH = 12;
constexpr int DOOR_MAP_HEIGHT = 9;
const Vector2D LOCKED_DOOR{ 3, 4 };
const Vector2D CLOSED_DOOR{ 6, 4 };
const Vector2D OPEN_DOOR{ 9, 4 };

// set_door is protected, so the fixture reaches it through this.
class MapWithDoors : public Map
{
public:
	using Map::Map;
	using Map::set_door;
};
} // namespace

class MapDoorPersistenceTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		original.init_tiles();
		// A floor row for the three doors to sit in.
		for (int col = 1; col < DOOR_MAP_WIDTH - 1; ++col)
		{
			original.set_tile(Vector2D{ col, 4 }, TileType::FLOOR);
		}
		original.set_door(LOCKED_DOOR);
		ASSERT_TRUE(original.lock_door(LOCKED_DOOR));
		original.set_door(CLOSED_DOOR);
		original.set_tile(OPEN_DOOR, TileType::OPEN_DOOR);
	}

	// The map as a fresh one reads it back from its save.
	[[nodiscard]] Map reloaded()
	{
		nlohmann::json saved;
		original.save(saved);
		Map fresh{ DOOR_MAP_WIDTH, DOOR_MAP_HEIGHT };
		fresh.load(saved);
		return fresh;
	}

	MapWithDoors original{ DOOR_MAP_WIDTH, DOOR_MAP_HEIGHT };
};

// The defect a player meets: a locked door, saved and loaded, is still locked.
TEST_F(MapDoorPersistenceTest, ALockedDoorIsStillLockedAfterASaveAndALoad)
{
	Map fresh = reloaded();

	EXPECT_EQ(fresh.get_tile_type(LOCKED_DOOR), TileType::CLOSED_DOOR);
	EXPECT_TRUE(fresh.is_door_locked(LOCKED_DOOR));
}

// A closed door comes back closed and unlocked, and an open one open.
TEST_F(MapDoorPersistenceTest, ClosedAndOpenDoorsKeepTheirStateThroughASave)
{
	Map fresh = reloaded();

	EXPECT_EQ(fresh.get_tile_type(CLOSED_DOOR), TileType::CLOSED_DOOR);
	EXPECT_FALSE(fresh.is_door_locked(CLOSED_DOOR));
	EXPECT_EQ(fresh.get_tile_type(OPEN_DOOR), TileType::OPEN_DOOR);
	EXPECT_FALSE(fresh.is_door_locked(OPEN_DOOR));
}

// A failed attempt at forcing survives a save, so a load does not give the attempt back.
TEST_F(MapDoorPersistenceTest, ADoorThatResistedForcingStillHasAfterASaveAndALoad)
{
	original.mark_resisted_forcing(LOCKED_DOOR);

	Map fresh = reloaded();

	EXPECT_TRUE(fresh.has_resisted_forcing(LOCKED_DOOR));
	EXPECT_TRUE(fresh.is_door_locked(LOCKED_DOOR));
}

// A failed pick survives a save, so a load does not give the level's attempt back.
TEST_F(MapDoorPersistenceTest, AFailedPickIsStillRememberedAfterASaveAndALoad)
{
	original.mark_lock_pick_failed(LOCKED_DOOR, 4);

	Map fresh = reloaded();

	EXPECT_EQ(fresh.lock_pick_failed_at_level(LOCKED_DOOR), 4);
	EXPECT_EQ(fresh.lock_pick_failed_at_level(CLOSED_DOOR), 0);
}

// The door pass runs when a level is generated, never on a load. So a corridor leaving a room
// through a one-cell gap walled on both sides - the shape that pass turns into a door - is
// still corridor after a save and a load, on a map built by hand rather than from a seed.
TEST_F(MapDoorPersistenceTest, AFreshlyCarvedExitIsStillACorridorAfterALoad)
{
	MapWithDoors carved{ DOOR_MAP_WIDTH, DOOR_MAP_HEIGHT };
	carved.init_tiles();
	// A room over columns 1-3 and rows 2-6, so the gap's west side is floor and its corners are too.
	for (int row = 2; row <= 6; ++row)
	{
		for (int col = 1; col <= 3; ++col)
		{
			carved.set_tile(Vector2D{ col, row }, TileType::FLOOR);
		}
	}
	const Vector2D gap{ 4, 4 };
	for (int col = gap.x; col <= gap.x + 3; ++col)
	{
		carved.set_tile(Vector2D{ col, gap.y }, TileType::CORRIDOR);
	}
	nlohmann::json saved;
	carved.save(saved);

	Map fresh{ DOOR_MAP_WIDTH, DOOR_MAP_HEIGHT };
	fresh.load(saved);

	EXPECT_EQ(fresh.get_tile_type(gap), TileType::CORRIDOR) << "the door pass ran on a load";
}

// Saving the loaded map writes what it was loaded from, door states included.
TEST_F(MapDoorPersistenceTest, ALoadedMapSavesWhatItWasLoadedFrom)
{
	nlohmann::json saved;
	original.save(saved);

	Map fresh = reloaded();
	nlohmann::json savedAgain;
	fresh.save(savedAgain);

	EXPECT_EQ(savedAgain, saved);
}

// A tile record without its door state is refused, naming the field.
TEST_F(MapDoorPersistenceTest, ATileWithoutItsDoorStateIsRefused)
{
	nlohmann::json saved;
	original.save(saved);
	for (nlohmann::json& tile : saved.at("tiles"))
	{
		tile.erase("doorState");
	}
	Map fresh{ DOOR_MAP_WIDTH, DOOR_MAP_HEIGHT };

	try
	{
		fresh.load(saved);
		ADD_FAILURE() << "a tile without its door state was loaded";
	}
	catch (const nlohmann::json::exception& refusal)
	{
		EXPECT_NE(std::string(refusal.what()).find("doorState"), std::string::npos) << refusal.what();
	}
}

// A tile record without its failed-pick level is refused, naming the field.
TEST_F(MapDoorPersistenceTest, ATileWithoutItsFailedPickLevelIsRefused)
{
	nlohmann::json saved;
	original.save(saved);
	for (nlohmann::json& tile : saved.at("tiles"))
	{
		tile.erase("lockPickFailedAtLevel");
	}
	Map fresh{ DOOR_MAP_WIDTH, DOOR_MAP_HEIGHT };

	try
	{
		fresh.load(saved);
		ADD_FAILURE() << "a tile without its failed-pick level was loaded";
	}
	catch (const nlohmann::json::exception& refusal)
	{
		EXPECT_NE(std::string(refusal.what()).find("lockPickFailedAtLevel"), std::string::npos) << refusal.what();
	}
}

// Counting up from 0, every door state below the first one refused loads and saves back as
// itself. No DoorState is named, so a state added later needs no edit here.
TEST_F(MapDoorPersistenceTest, EveryDoorStateBelowTheFirstRefusedOneRoundTrips)
{
	constexpr int SEARCH_CEILING = 64;
	std::optional<int> firstRefused{};
	for (int state = 0; state < SEARCH_CEILING; ++state)
	{
		nlohmann::json saved;
		original.save(saved);
		saved.at("tiles").at(0).at("doorState") = state;
		Map fresh{ DOOR_MAP_WIDTH, DOOR_MAP_HEIGHT };
		try
		{
			fresh.load(saved);
		}
		catch (const std::runtime_error&)
		{
			firstRefused = state;
			break;
		}

		nlohmann::json savedAgain;
		fresh.save(savedAgain);
		EXPECT_EQ(savedAgain.at("tiles").at(0).at("doorState"), state) << "door state " << state << " loaded as another";
	}
	ASSERT_TRUE(firstRefused.has_value()) << "no door state below " << SEARCH_CEILING << " was refused";
	EXPECT_GT(*firstRefused, 0) << "not even the first door state loaded";

	// The refusal names the value, past the last and below the first alike.
	for (const int corrupt : { *firstRefused, -1 })
	{
		nlohmann::json saved;
		original.save(saved);
		saved.at("tiles").at(0).at("doorState") = corrupt;
		Map fresh{ DOOR_MAP_WIDTH, DOOR_MAP_HEIGHT };

		try
		{
			fresh.load(saved);
			ADD_FAILURE() << "door state " << corrupt << " was loaded";
		}
		catch (const std::runtime_error& refusal)
		{
			const std::string reason = refusal.what();
			EXPECT_NE(reason.find(std::format("door state {} ", corrupt)), std::string::npos) << reason;
			EXPECT_NE(reason.find("is not a DoorState"), std::string::npos) << reason;
		}
	}
}
