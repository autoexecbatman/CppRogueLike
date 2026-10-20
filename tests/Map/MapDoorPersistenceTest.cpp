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

// A saved door state that no DoorState has - one past the last, or below the first - is a
// corrupt save, refused with the value named rather than loaded as a state nothing handles.
TEST_F(MapDoorPersistenceTest, ADoorStateNoDoorStateHasIsRefused)
{
	const int pastTheLast = static_cast<int>(DoorState::CLOSED_LOCKED) + 1;
	for (const int corrupt : { pastTheLast, -1 })
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
