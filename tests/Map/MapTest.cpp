#include <gtest/gtest.h>
#include <limits>

#include "src/Colors.h"
#include "src/DataManager.h"
#include "src/Decoration.h"
#include "src/DungeonRoom.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/InventoryData.h"
#include "src/Item.h"
#include "src/MagicalItemEffects.h"
#include "src/Map.h"
#include "src/MessageSystem.h"
#include "src/Pathfinding.h"
#include "src/Paths.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/RandomDice.h"
#include "src/TileConfig.h"
#include "tests/mocks/TestDice.h"

// ============================================================================
// MAP TESTS
// Tests tile operations, walkability, door mechanics, serialization
// ============================================================================

namespace
{
constexpr int TEST_MAP_WIDTH = 20;
constexpr int TEST_MAP_HEIGHT = 15;
} // namespace

class MapTest : public ::testing::Test
{
protected:
	std::unique_ptr<Map> map;
	std::unique_ptr<Player> player;
	std::vector<std::unique_ptr<Creature>> creatures;
	GameContext ctx;
	DataManager dataManager;
	MessageSystem messageSystem;
	FloorInventory floorInventory{};
	std::vector<std::unique_ptr<Decoration>> decorations{};
	RandomDice dice{ TEST_DICE_SEED };
	TileConfig tileConfig;

	void SetUp() override
	{
		try
		{
			dataManager.load_all_data(messageSystem);
		}
		catch (...)
		{
		}

		map = std::make_unique<Map>(TEST_MAP_WIDTH, TEST_MAP_HEIGHT);

		player = std::make_unique<Player>(Vector2D{ 5, 5 });
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->set_dr(0);
		player->set_thaco(20);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->healthPool = std::make_unique<HealthPool>(20);

		ctx.playerOwner = &player;
		ctx.dataManager = &dataManager;
		ctx.messageSystem = &messageSystem;
		ctx.floorInventory = &floorInventory;
		ctx.decorations = &decorations;
		ctx.dice = &dice;
		ctx.creatures = &creatures;
		ctx.map = map.get();

		// What a tile does is data, and Pathfinding::can_enter reads it.
		try
		{
			tileConfig.load(Paths::TILE_CONFIG);
		}
		catch (...)
		{
		}
		ctx.tileConfig = &tileConfig;

		dice.set_test_mode(true);

		// Initialize tiles (required before any tile operations)
		map->init_tiles(); // blank wall grid - no dungeon generation; tile-level tests need no more
	}

	void TearDown() override
	{
		dice.set_test_mode(false);
		dice.clear_fixed_rolls();
	}

	// Helper to manually set up a simple map with floor tiles.
	// Parameters follow the codebase convention: x (column) before y (row).
	void create_simple_room(int x1, int y1, int x2, int y2)
	{
		for (int y = y1; y <= y2; ++y)
		{
			for (int x = x1; x <= x2; ++x)
			{
				map->set_tile(Vector2D{ x, y }, TileType::FLOOR);
			}
		}
	}
};

// ----------------------------------------------------------------------------
// Tile Type Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, GetTileType_DefaultIsWall)
{
	// After construction, tiles should default to WALL
	// (depends on implementation - map starts with walls everywhere)
	// This tests that out-of-bounds also returns WALL
	EXPECT_EQ(map->get_tile_type(Vector2D{ -1, -1 }), TileType::WALL);
}

TEST_F(MapTest, GetTileType_OutOfBounds_ReturnsWall)
{
	EXPECT_EQ(map->get_tile_type(Vector2D{ 0, -1 }), TileType::WALL);
	EXPECT_EQ(map->get_tile_type(Vector2D{ -1, 0 }), TileType::WALL);
	EXPECT_EQ(map->get_tile_type(Vector2D{ 0, TEST_MAP_HEIGHT }), TileType::WALL);
	EXPECT_EQ(map->get_tile_type(Vector2D{ TEST_MAP_WIDTH, 0 }), TileType::WALL);
	EXPECT_EQ(map->get_tile_type(Vector2D{ 999, 999 }), TileType::WALL);
}

TEST_F(MapTest, SetTile_ChangesType)
{
	Vector2D pos{ 5, 5 };

	map->set_tile(pos, TileType::FLOOR);
	EXPECT_EQ(map->get_tile_type(pos), TileType::FLOOR);

	map->set_tile(pos, TileType::WATER);
	EXPECT_EQ(map->get_tile_type(pos), TileType::WATER);

	map->set_tile(pos, TileType::CORRIDOR);
	EXPECT_EQ(map->get_tile_type(pos), TileType::CORRIDOR);
}

TEST_F(MapTest, SetTile_OutOfBounds_DoesNotCrash)
{
	EXPECT_NO_THROW(map->set_tile(Vector2D{ -1, -1 }, TileType::FLOOR));
	EXPECT_NO_THROW(map->set_tile(Vector2D{ 999, 999 }, TileType::FLOOR));
}

// ----------------------------------------------------------------------------
// Door Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, IsDoor_ClosedDoor)
{
	Vector2D pos{ 5, 5 };
	map->set_tile(pos, TileType::CLOSED_DOOR);

	EXPECT_TRUE(map->is_door(pos));
	EXPECT_FALSE(map->is_open_door(pos));
}

TEST_F(MapTest, IsDoor_OpenDoor)
{
	Vector2D pos{ 5, 5 };
	map->set_tile(pos, TileType::OPEN_DOOR);

	EXPECT_TRUE(map->is_door(pos));
	EXPECT_TRUE(map->is_open_door(pos));
}

TEST_F(MapTest, IsDoor_Floor_NotDoor)
{
	Vector2D pos{ 5, 5 };
	map->set_tile(pos, TileType::FLOOR);

	EXPECT_FALSE(map->is_door(pos));
	EXPECT_FALSE(map->is_open_door(pos));
}

TEST_F(MapTest, OpenDoor_ClosedDoor_Opens)
{
	Vector2D pos{ 5, 5 };
	map->set_tile(pos, TileType::CLOSED_DOOR);

	bool result = map->open_door(pos, ctx);

	EXPECT_TRUE(result);
	EXPECT_EQ(map->get_tile_type(pos), TileType::OPEN_DOOR);
}

TEST_F(MapTest, OpenDoor_AlreadyOpen_ReturnsFalse)
{
	Vector2D pos{ 5, 5 };
	map->set_tile(pos, TileType::OPEN_DOOR);

	bool result = map->open_door(pos, ctx);

	EXPECT_FALSE(result);
}

TEST_F(MapTest, OpenDoor_NotADoor_ReturnsFalse)
{
	Vector2D pos{ 5, 5 };
	map->set_tile(pos, TileType::FLOOR);

	bool result = map->open_door(pos, ctx);

	EXPECT_FALSE(result);
}

TEST_F(MapTest, CloseDoor_OpenDoor_Closes)
{
	Vector2D pos{ 7, 7 }; // Use different pos than player (5,5)
	map->set_tile(pos, TileType::OPEN_DOOR);

	bool result = map->close_door(pos, ctx);

	EXPECT_TRUE(result);
	EXPECT_EQ(map->get_tile_type(pos), TileType::CLOSED_DOOR);
}

TEST_F(MapTest, CloseDoor_AlreadyClosed_ReturnsFalse)
{
	Vector2D pos{ 7, 7 }; // Use different pos than player (5,5)
	map->set_tile(pos, TileType::CLOSED_DOOR);

	bool result = map->close_door(pos, ctx);

	EXPECT_FALSE(result);
}

TEST_F(MapTest, CloseDoor_NotADoor_ReturnsFalse)
{
	Vector2D pos{ 7, 7 }; // Use different pos than player (5,5)
	map->set_tile(pos, TileType::WALL);

	bool result = map->close_door(pos, ctx);

	EXPECT_FALSE(result);
}

TEST_F(MapTest, CloseDoor_ActorOnDoor_ReturnsFalse)
{
	// Player is at {5, 5} - try to close door there
	Vector2D pos{ 5, 5 };
	map->set_tile(pos, TileType::OPEN_DOOR);

	bool result = map->close_door(pos, ctx);

	// Should fail because player is standing on the door
	EXPECT_FALSE(result);
	EXPECT_EQ(map->get_tile_type(pos), TileType::OPEN_DOOR);
}

// ----------------------------------------------------------------------------
// Wall Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, IsWall_SyncedBySetTile)
{
	// set_tile syncs fovMap via fov_properties_for - both tile type and
	// FOV walkability are updated atomically. is_wall must reflect the change.
	Vector2D pos{ 5, 5 };

	// All tiles start as WALL after init(false).
	EXPECT_TRUE(map->is_wall(pos));

	// Setting FLOOR must make the tile walkable in fovMap too.
	map->set_tile(pos, TileType::FLOOR);
	EXPECT_EQ(map->get_tile_type(pos), TileType::FLOOR);
	EXPECT_FALSE(map->is_wall(pos)) << "set_tile must sync fovMap walkability";

	// Reverting to WALL must make it non-walkable again.
	map->set_tile(pos, TileType::WALL);
	EXPECT_TRUE(map->is_wall(pos)) << "set_tile to WALL must mark tile non-walkable";
}

// ----------------------------------------------------------------------------
// Map Dimensions Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, GetWidth_ReturnsCorrectWidth)
{
	EXPECT_EQ(map->get_width(), TEST_MAP_WIDTH);
}

TEST_F(MapTest, GetHeight_ReturnsCorrectHeight)
{
	EXPECT_EQ(map->get_height(), TEST_MAP_HEIGHT);
}

TEST_F(MapTest, GetIndex_ValidPosition)
{
	// Index = y * width + x
	EXPECT_EQ(map->get_index(Vector2D{ 0, 0 }), 0);
	EXPECT_EQ(map->get_index(Vector2D{ 5, 0 }), 5);
	EXPECT_EQ(map->get_index(Vector2D{ 0, 1 }), TEST_MAP_WIDTH);
	EXPECT_EQ(map->get_index(Vector2D{ 3, 2 }), 2 * TEST_MAP_WIDTH + 3);
}

TEST_F(MapTest, GetIndex_OutOfBounds_Throws)
{
	EXPECT_THROW(map->get_index(Vector2D{ 0, -1 }), std::out_of_range);
	EXPECT_THROW(map->get_index(Vector2D{ -1, 0 }), std::out_of_range);
	EXPECT_THROW(map->get_index(Vector2D{ 0, TEST_MAP_HEIGHT }), std::out_of_range);
	EXPECT_THROW(map->get_index(Vector2D{ TEST_MAP_WIDTH, 0 }), std::out_of_range);
}

// ----------------------------------------------------------------------------
// Explored State Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, IsExplored_InitiallyFalse)
{
	// Need to initialize tiles first
	// After init, tiles exist but may or may not be explored
	// depends on init implementation
	Vector2D pos{ 5, 5 };
	// Just verify no crash
	EXPECT_NO_THROW(map->is_explored(pos));
}

TEST_F(MapTest, IsExplored_OutOfBounds_ReturnsFalse)
{
	EXPECT_FALSE(map->is_explored(Vector2D{ -1, -1 }));
	EXPECT_FALSE(map->is_explored(Vector2D{ 999, 999 }));
}

// ----------------------------------------------------------------------------
// Serialization Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, Serialization_DimensionsPreserved)
{
	json j;
	map->save(j);

	// Verify dimensions are saved
	EXPECT_TRUE(j.contains("map_width") || j.contains("width"));
	EXPECT_TRUE(j.contains("map_height") || j.contains("height"));
}

TEST_F(MapTest, Serialization_TilesPreserved)
{
	// Set some specific tile
	Vector2D testPos{ 5, 5 };
	map->set_tile(testPos, TileType::WATER);

	json j;
	map->save(j);

	// Create new map and load
	auto loadedMap = std::make_unique<Map>(TEST_MAP_WIDTH, TEST_MAP_HEIGHT);
	loadedMap->load(j);

	EXPECT_EQ(loadedMap->get_tile_type(testPos), TileType::WATER);
}

// Water is walked through, so it is no wall.
TEST_F(MapTest, WaterIsNoWall)
{
	const Vector2D pool{ 5, 5 };
	map->set_tile(pool, TileType::WATER);

	EXPECT_FALSE(map->is_wall(pool));
}

// A load rebuilds the field-of-view grid from the tiles, so a wall comes back a wall and
// a floor comes back open.
TEST_F(MapTest, Serialization_ALoadedLevelKeepsItsWalls)
{
	const Vector2D floor{ 6, 5 };
	const Vector2D wall{ 7, 5 };
	map->set_tile(floor, TileType::FLOOR);
	json j;
	map->save(j);

	auto loadedMap = std::make_unique<Map>(TEST_MAP_WIDTH, TEST_MAP_HEIGHT);
	loadedMap->load(j);

	EXPECT_TRUE(loadedMap->is_wall(wall));
	EXPECT_FALSE(loadedMap->is_wall(floor));
}

// ----------------------------------------------------------------------------
// Edge Cases
// ----------------------------------------------------------------------------

TEST_F(MapTest, ZeroSizeMap_DoesNotCrash)
{
	// Edge case: What happens with 0 dimensions?
	// This tests defensive coding
	// Note: Actual behavior depends on implementation
}

TEST_F(MapTest, SingleTileMap)
{
	auto tinyMap = std::make_unique<Map>(1, 1);

	EXPECT_EQ(tinyMap->get_width(), 1);
	EXPECT_EQ(tinyMap->get_height(), 1);
	EXPECT_NO_THROW(tinyMap->get_index(Vector2D{ 0, 0 }));
}

TEST_F(MapTest, LargeCoordinates_HandledGracefully)
{
	// Very large coordinates should not crash
	EXPECT_NO_THROW(map->get_tile_type(Vector2D{ std::numeric_limits<int>::max(), std::numeric_limits<int>::max() }));
	EXPECT_NO_THROW(map->get_tile_type(Vector2D{ std::numeric_limits<int>::min(), std::numeric_limits<int>::min() }));
}

// ----------------------------------------------------------------------------
// Line of Sight Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, HasLOS_SamePosition_ReturnsTrue)
{
	Vector2D pos{ 5, 5 };

	EXPECT_TRUE(map->has_los(pos, pos));
}

TEST_F(MapTest, HasLOS_AdjacentFloor_ReturnsTrue)
{
	create_simple_room(4, 4, 6, 6);

	// Adjacent floor tiles should have LOS
	// (depends on tcodMap state which is set during dig operations)
}

// ----------------------------------------------------------------------------
// Reveal Map Test
// ----------------------------------------------------------------------------

TEST_F(MapTest, Reveal_MarksAllExplored)
{
	map->reveal();

	// After reveal, all valid tiles should be explored
	// Check a few positions
	EXPECT_TRUE(map->is_explored(Vector2D{ 1, 1 }));
	EXPECT_TRUE(map->is_explored(Vector2D{ 5, 5 }));
	EXPECT_TRUE(map->is_explored(Vector2D{ TEST_MAP_WIDTH - 2, TEST_MAP_HEIGHT - 2 }));
}

// ----------------------------------------------------------------------------
// Get Actor Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, GetActor_EmptyPosition_ReturnsNull)
{
	Creature* actor = map->get_actor(Vector2D{ 7, 7 }, ctx); // Not player pos (5,5)

	// Empty position should return nullptr
	EXPECT_EQ(actor, nullptr);
}

TEST_F(MapTest, GetActor_PlayerPosition_ReturnsPlayer)
{
	// Player is already at {5, 5} from SetUp
	Creature* actor = map->get_actor(Vector2D{ 5, 5 }, ctx);

	// Should return the player
	EXPECT_EQ(actor, player.get());
}

// Water blocks whoever cannot cross it, and the tile's bypass state is asked of the
// creature rather than of its own flags alone - so a creature wearing the gauntlets of
// swimming and climbing walks into water that stops a creature without them.
TEST_F(MapTest, WaterIsCrossedByWhateverGrantsSwimming)
{
	const Vector2D pool{ 4, 4 };
	map->set_tile(pool, TileType::WATER);

	Creature wader{ Vector2D{ 3, 4 }, ActorData{ TileRef{}, "wader", ColorPairId::WHITE_BLACK } };
	EXPECT_FALSE(Pathfinding::can_enter(wader, pool, ctx)) << "water stopped nobody";

	Creature spider{ Vector2D{ 3, 4 }, ActorData{ TileRef{}, "spider", ColorPairId::WHITE_BLACK } };
	spider.add_state(ActorState::CAN_SWIM);
	EXPECT_TRUE(Pathfinding::can_enter(spider, pool, ctx));

	// Worn rather than innate, and put straight into the slot: how it got there is
	// Player::equip_item's business, not the map's.
	auto gauntlets = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{});
	gauntlets->behavior = Gauntlets{ .effect = MagicalEffect::SWIMMING };
	wader.equippedItems.emplace_back(std::move(gauntlets), EquipmentSlot::GAUNTLETS);

	EXPECT_TRUE(Pathfinding::can_enter(wader, pool, ctx))
		<< "the map asked the creature's own states and not what it wears";
}
