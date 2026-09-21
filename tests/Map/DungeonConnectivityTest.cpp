// file: DungeonConnectivityTest.cpp
// A generated dungeon leaves nothing behind its own walls: every tile it carves can be
// walked to from where the player starts.
//
// What it is for. Three passes wrote into a room's bounding box without asking what was
// already there. `Map::spawn_water` painted pools into the cells a room's shape had
// walled back, sealed inside rock - 88 tiles in 200 maps. `TreasureRoom::create` filled
// the box with floor and flattened whichever shape the vault landed on - 8 rooms in 785.
// And a room takes both a prefab and a procedural shape, each carving walls the other
// cannot see, which could split a room in two or leave an edge with no floor for a
// corridor to enter by.
//
// **The start point is the player's own tile, never a room's centre.** A centre is a
// point of geometry: the moat prefab puts a solid block through the middle of its room,
// so the centre is rock, and a flood started there reports the whole map unreachable
// when nothing is wrong with it. That mistake is what made this assert fire in a real
// game.
//
// **The prefab library has to be loaded** or `Map::create_room` stamps nothing and the
// suite measures a generator no player runs.
//
// Reachability is terrain, not permission: a door is a way through whether it is open,
// shut or locked. Counting a closed door as a wall would report the locked treasure room,
// which is deliberate, as a defect.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=DungeonConnectivityTest.*

#include <gtest/gtest.h>

#include <memory>
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
#include "src/Paths.h"
#include "src/Player.h"
#include "src/PrefabLibrary.h"
#include "src/Stairs.h"
#include "src/TileType.h"
#include "src/Trap.h"
#include "tests/mocks/MockGameContext.h"

namespace
{

constexpr int MAP_WIDTH = 120;
constexpr int MAP_HEIGHT = 70;

// How many dungeons a case builds. The defect this pins appeared in about a third of
// maps, so one draw would have passed clean more often than not.
constexpr int DUNGEONS = 60;

} // namespace

class DungeonConnectivityTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// The generator needs real rolls; a queued-dice mock would build one fixed map.
		mock.dice.set_test_mode(false);

		// The game loads these in main.cpp, and create_room stamps a prefab only when
		// both are present: without them the generator takes a path no player ever sees.
		prefabs.load_tile_labels(Paths::TILE_CONFIG);
		prefabs.load(Paths::PREFABS);
		// A loader that silently opens nothing leaves this suite measuring a generator
		// no player runs - the failure that kept CI red for 38 pushes, one layer down.
		ASSERT_GT(prefabs.count(), 0) << "no prefabs loaded; every room would be a bare box";

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
		ctx.decorEditor = &decorEditor;
		ctx.creatures = &creatures;
		ctx.levelManager = &levelManager;
		ctx.traps = &traps;
		ctx.decorations = &decorations;
		ctx.floorInventory = &floor;
	}

	// Builds one dungeon into a fresh map and returns it, with everything it spawned
	// cleared first so one run cannot fill the next one's floor.
	void generate_into(Map& map)
	{
		ctx.map = &map;
		rooms.clear();
		creatures.clear();
		traps.clear();
		decorations.clear();
		floor.items.clear();
		map.init(ctx);
	}

	// Where the player actually stands. A room's centre is geometry, not floor - the moat
	// prefab puts a wall block straight through the middle of its room.
	Vector2D player_start() const
	{
		return player->position;
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> player{};
	LevelManager levelManager{};
	PrefabLibrary prefabs{};
	DecorEditor decorEditor{};
	std::vector<DungeonRoom> rooms{};
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::vector<std::unique_ptr<Trap>> traps{};
	std::vector<std::unique_ptr<Decoration>> decorations{};
	Stairs stairs{ Vector2D{ 1, 1 } };
	// Room enough for every dungeon a case builds; the mock's own floor is sized for one.
	FloorInventory floor{ 4000 };
};

TEST_F(DungeonConnectivityTest, EveryCarvedTileIsReachableFromTheEntrance)
{
	int strandedMaps = 0;
	int worstStranded = 0;

	for (int attempt = 0; attempt < DUNGEONS; ++attempt)
	{
		Map map{ MAP_WIDTH, MAP_HEIGHT };
		generate_into(map);
		ASSERT_FALSE(rooms.empty()) << "the generator produced no rooms at all";

		const int stranded = map.count_unreachable_tiles(player_start());
		ASSERT_GE(stranded, 0) << "the player was left standing somewhere that is not a tile";

		if (stranded > 0)
		{
			++strandedMaps;
			worstStranded = std::max(worstStranded, stranded);
		}
	}

	EXPECT_EQ(strandedMaps, 0)
		<< strandedMaps << " of " << DUNGEONS << " dungeons sealed carved tiles inside rock; "
		<< "the worst left " << worstStranded << " of them";
}

TEST_F(DungeonConnectivityTest, WaterOnlyEverReplacesFloorTheRoomCarved)
{
	// The defect behind the stranding, stated directly: every water tile has to sit
	// where a room or corridor put walkable ground, never in a shaped room's walled-back
	// corner. A water tile with wall on all four sides is that corner.
	for (int attempt = 0; attempt < DUNGEONS; ++attempt)
	{
		Map map{ MAP_WIDTH, MAP_HEIGHT };
		generate_into(map);

		for (int row = 1; row < MAP_HEIGHT - 1; ++row)
		{
			for (int col = 1; col < MAP_WIDTH - 1; ++col)
			{
				const Vector2D pos{ col, row };
				if (map.get_tile_type(pos) != TileType::WATER)
				{
					continue;
				}
				const bool walledIn =
					map.get_tile_type(Vector2D{ col + 1, row }) == TileType::WALL &&
					map.get_tile_type(Vector2D{ col - 1, row }) == TileType::WALL &&
					map.get_tile_type(Vector2D{ col, row + 1 }) == TileType::WALL &&
					map.get_tile_type(Vector2D{ col, row - 1 }) == TileType::WALL;
				ASSERT_FALSE(walledIn)
					<< "water at " << col << "," << row << " is sealed in rock on all four sides";
			}
		}
	}
}

TEST_F(DungeonConnectivityTest, AStartOffTheMapIsRefusedRatherThanReportedClean)
{
	Map map{ MAP_WIDTH, MAP_HEIGHT };
	generate_into(map);

	// The border ring is always wall, so this is a question the map cannot answer.
	EXPECT_EQ(map.count_unreachable_tiles(Vector2D{ 0, 0 }), -1);
	EXPECT_EQ(map.count_unreachable_tiles(Vector2D{ -1, -1 }), -1);
}
