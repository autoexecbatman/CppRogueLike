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

#include <cassert>
#include <memory>
#include <optional>
#include <vector>

#include "src/ArmorClass.h"
#include "src/Colors.h"
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
#include "src/Pathfinding.h"
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
		palette.load(Paths::TILE_CONFIG);
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
		ctx.palette = &palette;
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

// The cost maps monsters descend promise nothing the step refuses. Every tile a map
// reaches may itself be entered by that mobility, and has a cheaper neighbour it may also
// enter, so a monster walking downhill from anywhere the map reaches is never led to a
// tile its own step turns away.
TEST_F(DungeonConnectivityTest, EveryTileACostMapReachesLeadsDownToTheGoal)
{
	Creature goblin{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK } };
	goblin.set_body_plan(mock.body_plans.get("humanoid"));
	Creature wolf{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "wolf", ColorPairId::WHITE_BLACK } };
	wolf.set_body_plan(mock.body_plans.get("beast"));
	const std::vector<Pathfinding::Mobility> mobilities{
		Pathfinding::mobility_of(goblin, mock.tile_config),
		Pathfinding::mobility_of(wolf, mock.tile_config)
	};

	int unenterable = 0;
	int deadEnds = 0;
	for (int attempt = 0; attempt < DUNGEONS; ++attempt)
	{
		Map map{ MAP_WIDTH, MAP_HEIGHT };
		generate_into(map);
		const Vector2D goal = player_start();

		for (const Pathfinding::Mobility& mobility : mobilities)
		{
			for (int row = 1; row < MAP_HEIGHT - 1; ++row)
			{
				for (int col = 1; col < MAP_WIDTH - 1; ++col)
				{
					const Vector2D tile{ col, row };
					const std::optional<int> cost = mock.pathfinder.cost_to(goal, tile, mobility, ctx);
					if (!cost || tile == goal)
					{
						continue;
					}
					if (!Pathfinding::step_cost(tile, mobility, ctx))
					{
						++unenterable;
					}

					bool hasWayDown = false;
					for (int rowStep = -1; rowStep <= 1; ++rowStep)
					{
						for (int colStep = -1; colStep <= 1; ++colStep)
						{
							const Vector2D next{ col + colStep, row + rowStep };
							const std::optional<int> nextCost = mock.pathfinder.cost_to(goal, next, mobility, ctx);
							const bool isEnterable = next == goal || Pathfinding::step_cost(next, mobility, ctx).has_value();
							if (nextCost && *nextCost < *cost && isEnterable)
							{
								hasWayDown = true;
							}
						}
					}
					if (!hasWayDown)
					{
						++deadEnds;
					}
				}
			}
		}
	}

	EXPECT_EQ(unenterable, 0) << "a cost map reached tiles its own mobility may not enter";
	EXPECT_EQ(deadEnds, 0) << "a cost map left tiles with no way down a monster may take";
}

// Nothing a level is generated with stands in water, because the character a game starts
// with cannot go in after it: no barrel, no item on the floor, not the stairs and not the
// player.
TEST_F(DungeonConnectivityTest, NothingIsGeneratedStandingInWater)
{
	int wetBarrels = 0;
	int wetItems = 0;
	int wetStairs = 0;
	int wetPlayers = 0;
	for (int attempt = 0; attempt < DUNGEONS; ++attempt)
	{
		Map map{ MAP_WIDTH, MAP_HEIGHT };
		generate_into(map);

		for (const auto& decoration : decorations)
		{
			assert(decoration && "a generated level holds a null decoration");
			wetBarrels += map.is_water(decoration->position) ? 1 : 0;
		}
		for (const auto& item : floor.items)
		{
			assert(item && "a generated level holds a null item");
			wetItems += map.is_water(item->position) ? 1 : 0;
		}
		wetStairs += map.is_water(stairs.position) ? 1 : 0;
		wetPlayers += map.is_water(player_start()) ? 1 : 0;
	}

	EXPECT_EQ(wetBarrels, 0) << "barrels generated in water across " << DUNGEONS << " dungeons";
	EXPECT_EQ(wetItems, 0) << "items generated in water";
	EXPECT_EQ(wetStairs, 0) << "stairs generated in water";
	EXPECT_EQ(wetPlayers, 0) << "a player started in water";
}

TEST_F(DungeonConnectivityTest, AStartOffTheMapIsRefusedRatherThanReportedClean)
{
	Map map{ MAP_WIDTH, MAP_HEIGHT };
	generate_into(map);

	// The border ring is always wall, so this is a question the map cannot answer.
	EXPECT_EQ(map.count_unreachable_tiles(Vector2D{ 0, 0 }), -1);
	EXPECT_EQ(map.count_unreachable_tiles(Vector2D{ -1, -1 }), -1);
}

// Monster movement relies on this rather than checking each neighbour against the map's
// edge: nothing walkable on the edge means nothing stands there.
TEST_F(DungeonConnectivityTest, EveryDungeonIsRingedByWall)
{
	for (int attempt = 0; attempt < DUNGEONS; ++attempt)
	{
		Map map{ MAP_WIDTH, MAP_HEIGHT };
		generate_into(map);
		ASSERT_TRUE(map.has_wall_border()) << "dungeon " << attempt << " carved into the map's edge";
	}
}

// One floor tile on each edge in turn, so a check that skips a side is caught on that side.
TEST_F(DungeonConnectivityTest, FloorOnAnyEdgeBreaksTheRing)
{
	const Vector2D edgeTiles[] = {
		Vector2D{ 5, 0 },
		Vector2D{ 5, MAP_HEIGHT - 1 },
		Vector2D{ 0, 5 },
		Vector2D{ MAP_WIDTH - 1, 5 },
	};

	for (const Vector2D& edgeTile : edgeTiles)
	{
		Map map{ MAP_WIDTH, MAP_HEIGHT };
		map.init_tiles();
		ASSERT_TRUE(map.has_wall_border()) << "a map of solid rock is ringed by wall";

		map.set_tile(edgeTile, TileType::FLOOR);
		EXPECT_FALSE(map.has_wall_border())
			<< "floor at (" << edgeTile.x << "," << edgeTile.y << ") was not seen";
	}
}

// Floor one tile in from every edge is the most a room may reach, and leaves the ring whole.
TEST_F(DungeonConnectivityTest, FloorJustInsideTheEdgeLeavesTheRingWhole)
{
	Map map{ MAP_WIDTH, MAP_HEIGHT };
	map.init_tiles();
	map.set_tile(Vector2D{ 1, 1 }, TileType::FLOOR);
	map.set_tile(Vector2D{ MAP_WIDTH - 2, MAP_HEIGHT - 2 }, TileType::FLOOR);

	EXPECT_TRUE(map.has_wall_border());
}
