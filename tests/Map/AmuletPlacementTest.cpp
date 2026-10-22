// file: AmuletPlacementTest.cpp
// The Amulet of Yendor is never generated somewhere the player cannot reach it.
//
// It is the win condition and it is placed once, on the final level, in a random room.
// Map::place_amulet rejected walls, occupied tiles and the staircase, and accepted
// water - and water bars anyone without CAN_SWIM, which no starting character has. An
// amulet in a pool is an unwinnable game, and nothing in a build, a suite or a normal
// run says so, because the loop that chose the tile was satisfied.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=AmuletPlacementTest.*

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <nlohmann/json.hpp>

#include "src/ArmorClass.h"
#include "src/DataManager.h"
#include "src/DungeonRoom.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/InventoryData.h"
#include "src/Item.h"
#include "src/LevelManager.h"
#include "src/Map.h"
#include "src/MessageSystem.h"
#include "src/Player.h"
#include "tests/mocks/MockGameContext.h"

namespace
{

constexpr int FIXTURE_WIDTH = 30;
constexpr int FIXTURE_HEIGHT = 20;

// A room whose floor is water except for one dry tile. Every attempt that reads the
// water as placeable lands in it, so one call is enough to show the defect and the
// repeats are there to make a pass mean something rather than to catch a rare case.
constexpr int ROOM_COL = 2;
constexpr int ROOM_ROW = 2;
constexpr int ROOM_WIDTH = 8;
constexpr int ROOM_HEIGHT = 6;
constexpr int DRY_COL = 5;
constexpr int DRY_ROW = 4;

constexpr int ATTEMPTS = 30;

} // namespace

class AmuletPlacementTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		try
		{
			dataManager.load_all_data(messageSystem);
		}
		catch (...)
		{
		}

		map = std::make_unique<Map>(FIXTURE_WIDTH, FIXTURE_HEIGHT);
		player = std::make_unique<Player>(Vector2D{ 1, 1 });
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->healthPool = std::make_unique<HealthPool>(20);

		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		ctx.dataManager = &dataManager;
		ctx.messageSystem = &messageSystem;
		ctx.creatures = &creatures;
		ctx.map = map.get();
		ctx.levelManager = &levelManager;
		ctx.rooms = &rooms;

		// The amulet is only placed on the last level, so the fixture has to be there.
		// LevelManager has no setter; its save format is the way in.
		nlohmann::json levelState;
		levelState["level_manager"]["dungeonLevel"] = FINAL_DUNGEON_LEVEL;
		levelState["level_manager"]["shopkeepersOnCurrentLevel"] = 0;
		levelManager.load_from_json(levelState);

		map->init_tiles();
		for (int row = ROOM_ROW; row < ROOM_ROW + ROOM_HEIGHT; ++row)
		{
			for (int col = ROOM_COL; col < ROOM_COL + ROOM_WIDTH; ++col)
			{
				map->set_tile(Vector2D{ col, row }, TileType::WATER);
			}
		}
		map->set_tile(Vector2D{ DRY_COL, DRY_ROW }, TileType::FLOOR);

		rooms.clear();
		rooms.push_back(DungeonRoom{ ROOM_COL, ROOM_ROW, ROOM_WIDTH, ROOM_HEIGHT });
	}

	// Where the amulet ended up, read back off the floor rather than off the call.
	Vector2D amulet_position() const
	{
		for (const auto& item : mock.inventory.items)
		{
			if (item && item->actorData.name.find("Amulet") != std::string::npos)
			{
				return item->position;
			}
		}
		return Vector2D{ -1, -1 };
	}

	MockGameContext mock{};
	GameContext ctx{};
	DataManager dataManager{};
	MessageSystem messageSystem{};
	LevelManager levelManager{};
	std::unique_ptr<Map> map{};
	std::unique_ptr<Player> player{};
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::vector<DungeonRoom> rooms{};
};

// The whole point: the one tile in that room a player without CAN_SWIM can stand on is
// the only tile the amulet may occupy.
TEST_F(AmuletPlacementTest, TheAmuletIsNeverPlacedInWater)
{
	for (int attempt = 0; attempt < ATTEMPTS; ++attempt)
	{
		mock.inventory.items.clear();

		map->place_amulet(ctx);

		const Vector2D placed = amulet_position();
		ASSERT_NE(placed.x, -1) << "attempt " << attempt << ": no amulet was placed at all";
		EXPECT_FALSE(map->is_water(placed))
			<< "attempt " << attempt << ": the amulet is in water at (" << placed.x << "," << placed.y << ")";
		EXPECT_EQ(placed.x, DRY_COL) << "attempt " << attempt << ": the only dry tile is the only legal one";
		EXPECT_EQ(placed.y, DRY_ROW);
	}
}
