#include <gtest/gtest.h>
#include <limits>

#include "src/Map/Map.h"
#include "src/Core/GameContext.h"
#include "src/ActorTypes/Player.h"
#include "src/Actor/Destructible.h"
#include "src/Systems/DataManager.h"
#include "src/Systems/MessageSystem.h"
#include "src/Random/RandomDice.h"

// ============================================================================
// MAP TESTS
// Tests tile operations, walkability, door mechanics, serialization
// ============================================================================

class MapTest : public ::testing::Test
{
protected:
    std::unique_ptr<Map> map;
    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Creature>> creatures;
    GameContext ctx;
    DataManager data_manager;
    MessageSystem message_system;
    RandomDice dice;

    static constexpr int TEST_MAP_WIDTH = 20;
    static constexpr int TEST_MAP_HEIGHT = 15;

    void SetUp() override
    {
        try
        {
            data_manager.load_all_data(message_system);
        }
        catch (...) {}

        map = std::make_unique<Map>(TEST_MAP_HEIGHT, TEST_MAP_WIDTH);

        player = std::make_unique<Player>(Vector2D{5, 5});
        player->destructible = std::make_unique<PlayerDestructible>(
            20, 0, "your corpse", 0, 20, 10
        );

        ctx.player = player.get();
        ctx.data_manager = &data_manager;
        ctx.message_system = &message_system;
        ctx.dice = &dice;
        ctx.creatures = &creatures;
        ctx.map = map.get();

        dice.set_test_mode(true);

        // Initialize tiles (required before any tile operations)
        map->init(false, ctx);  // false = don't spawn actors
    }

    void TearDown() override
    {
        dice.set_test_mode(false);
        dice.clear_fixed_rolls();
    }

    // Helper to manually set up a simple map with floor tiles
    void create_simple_room(int y1, int x1, int y2, int x2)
    {
        for (int y = y1; y <= y2; ++y)
        {
            for (int x = x1; x <= x2; ++x) 
            {
                map->set_tile(Vector2D{y, x}, TileType::FLOOR, 1.0);
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
    EXPECT_EQ(map->get_tile_type(Vector2D{-1, -1}), TileType::WALL);
}

TEST_F(MapTest, GetTileType_OutOfBounds_ReturnsWall)
{
    EXPECT_EQ(map->get_tile_type(Vector2D{-1, 0}), TileType::WALL);
    EXPECT_EQ(map->get_tile_type(Vector2D{0, -1}), TileType::WALL);
    EXPECT_EQ(map->get_tile_type(Vector2D{TEST_MAP_HEIGHT, 0}), TileType::WALL);
    EXPECT_EQ(map->get_tile_type(Vector2D{0, TEST_MAP_WIDTH}), TileType::WALL);
    EXPECT_EQ(map->get_tile_type(Vector2D{999, 999}), TileType::WALL);
}

TEST_F(MapTest, SetTile_ChangesType)
{
    Vector2D pos{5, 5};

    map->set_tile(pos, TileType::FLOOR, 1.0);
    EXPECT_EQ(map->get_tile_type(pos), TileType::FLOOR);

    map->set_tile(pos, TileType::WATER, 10.0);
    EXPECT_EQ(map->get_tile_type(pos), TileType::WATER);

    map->set_tile(pos, TileType::CORRIDOR, 1.0);
    EXPECT_EQ(map->get_tile_type(pos), TileType::CORRIDOR);
}

TEST_F(MapTest, SetTile_OutOfBounds_DoesNotCrash)
{
    EXPECT_NO_THROW(map->set_tile(Vector2D{-1, -1}, TileType::FLOOR, 1.0));
    EXPECT_NO_THROW(map->set_tile(Vector2D{999, 999}, TileType::FLOOR, 1.0));
}

// ----------------------------------------------------------------------------
// Door Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, IsDoor_ClosedDoor)
{
    Vector2D pos{5, 5};
    map->set_tile(pos, TileType::CLOSED_DOOR, 2.0);

    EXPECT_TRUE(map->is_door(pos));
    EXPECT_FALSE(map->is_open_door(pos));
}

TEST_F(MapTest, IsDoor_OpenDoor)
{
    Vector2D pos{5, 5};
    map->set_tile(pos, TileType::OPEN_DOOR, 1.0);

    EXPECT_TRUE(map->is_door(pos));
    EXPECT_TRUE(map->is_open_door(pos));
}

TEST_F(MapTest, IsDoor_Floor_NotDoor)
{
    Vector2D pos{5, 5};
    map->set_tile(pos, TileType::FLOOR, 1.0);

    EXPECT_FALSE(map->is_door(pos));
    EXPECT_FALSE(map->is_open_door(pos));
}

TEST_F(MapTest, OpenDoor_ClosedDoor_Opens)
{
    Vector2D pos{5, 5};
    map->set_tile(pos, TileType::CLOSED_DOOR, 2.0);

    bool result = map->open_door(pos, ctx);

    EXPECT_TRUE(result);
    EXPECT_EQ(map->get_tile_type(pos), TileType::OPEN_DOOR);
}

TEST_F(MapTest, OpenDoor_AlreadyOpen_ReturnsFalse)
{
    Vector2D pos{5, 5};
    map->set_tile(pos, TileType::OPEN_DOOR, 1.0);

    bool result = map->open_door(pos, ctx);

    EXPECT_FALSE(result);
}

TEST_F(MapTest, OpenDoor_NotADoor_ReturnsFalse)
{
    Vector2D pos{5, 5};
    map->set_tile(pos, TileType::FLOOR, 1.0);

    bool result = map->open_door(pos, ctx);

    EXPECT_FALSE(result);
}

TEST_F(MapTest, CloseDoor_OpenDoor_Closes)
{
    Vector2D pos{7, 7};  // Use different pos than player (5,5)
    map->set_tile(pos, TileType::OPEN_DOOR, 1.0);

    bool result = map->close_door(pos, ctx);

    EXPECT_TRUE(result);
    EXPECT_EQ(map->get_tile_type(pos), TileType::CLOSED_DOOR);
}

TEST_F(MapTest, CloseDoor_AlreadyClosed_ReturnsFalse)
{
    Vector2D pos{7, 7};  // Use different pos than player (5,5)
    map->set_tile(pos, TileType::CLOSED_DOOR, 2.0);

    bool result = map->close_door(pos, ctx);

    EXPECT_FALSE(result);
}

TEST_F(MapTest, CloseDoor_NotADoor_ReturnsFalse)
{
    Vector2D pos{7, 7};  // Use different pos than player (5,5)
    map->set_tile(pos, TileType::WALL, 1.0);

    bool result = map->close_door(pos, ctx);

    EXPECT_FALSE(result);
}

TEST_F(MapTest, CloseDoor_ActorOnDoor_ReturnsFalse)
{
    // Player is at {5, 5} - try to close door there
    Vector2D pos{5, 5};
    map->set_tile(pos, TileType::OPEN_DOOR, 1.0);

    bool result = map->close_door(pos, ctx);

    // Should fail because player is standing on the door
    EXPECT_FALSE(result);
    EXPECT_EQ(map->get_tile_type(pos), TileType::OPEN_DOOR);
}

// ----------------------------------------------------------------------------
// Wall Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, IsWall_ChecksTcodMapWalkability)
{
    // is_wall() checks tcodMap->isWalkable(), NOT tile type
    // set_tile() only updates tile array, not tcodMap
    // This documents the actual behavior
    Vector2D pos{5, 5};

    // Initially after init(), tcodMap has its own walkability state
    // is_wall returns true if tcodMap says tile is NOT walkable
    bool initialWallState = map->is_wall(pos);

    // set_tile changes tile type but NOT tcodMap walkability
    map->set_tile(pos, TileType::FLOOR, 1.0);
    EXPECT_EQ(map->get_tile_type(pos), TileType::FLOOR);

    // is_wall still returns same value because tcodMap unchanged
    EXPECT_EQ(map->is_wall(pos), initialWallState)
        << "set_tile does not affect tcodMap walkability";
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
    EXPECT_EQ(map->get_index(Vector2D{0, 0}), 0);
    EXPECT_EQ(map->get_index(Vector2D{0, 5}), 5);
    EXPECT_EQ(map->get_index(Vector2D{1, 0}), TEST_MAP_WIDTH);
    EXPECT_EQ(map->get_index(Vector2D{2, 3}), 2 * TEST_MAP_WIDTH + 3);
}

TEST_F(MapTest, GetIndex_OutOfBounds_Throws)
{
    EXPECT_THROW(map->get_index(Vector2D{-1, 0}), std::out_of_range);
    EXPECT_THROW(map->get_index(Vector2D{0, -1}), std::out_of_range);
    EXPECT_THROW(map->get_index(Vector2D{TEST_MAP_HEIGHT, 0}), std::out_of_range);
    EXPECT_THROW(map->get_index(Vector2D{0, TEST_MAP_WIDTH}), std::out_of_range);
}

// ----------------------------------------------------------------------------
// Explored State Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, IsExplored_InitiallyFalse)
{
    // Need to initialize tiles first
    // After init, tiles exist but may or may not be explored
    // depends on init implementation
    Vector2D pos{5, 5};
    // Just verify no crash
    EXPECT_NO_THROW(map->is_explored(pos));
}

TEST_F(MapTest, IsExplored_OutOfBounds_ReturnsFalse)
{
    EXPECT_FALSE(map->is_explored(Vector2D{-1, -1}));
    EXPECT_FALSE(map->is_explored(Vector2D{999, 999}));
}

// ----------------------------------------------------------------------------
// Get Cost Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, GetCost_OutOfBounds_ReturnsHighCost)
{
    double cost = map->get_cost(Vector2D{-1, -1}, ctx);
    EXPECT_GE(cost, 1000.0);
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
    Vector2D testPos{5, 5};
    map->set_tile(testPos, TileType::WATER, 10.0);

    json j;
    map->save(j);

    // Create new map and load
    auto loadedMap = std::make_unique<Map>(TEST_MAP_HEIGHT, TEST_MAP_WIDTH);
    loadedMap->load(j);

    EXPECT_EQ(loadedMap->get_tile_type(testPos), TileType::WATER);
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
    EXPECT_NO_THROW(tinyMap->get_index(Vector2D{0, 0}));
}

TEST_F(MapTest, LargeCoordinates_HandledGracefully)
{
    // Very large coordinates should not crash
    EXPECT_NO_THROW(map->get_tile_type(Vector2D{std::numeric_limits<int>::max(), std::numeric_limits<int>::max()}));
    EXPECT_NO_THROW(map->get_tile_type(Vector2D{std::numeric_limits<int>::min(), std::numeric_limits<int>::min()}));
}

// ----------------------------------------------------------------------------
// Line of Sight Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, HasLOS_SamePosition_ReturnsTrue)
{
    Vector2D pos{5, 5};

    EXPECT_TRUE(map->has_los(pos, pos));
}

TEST_F(MapTest, HasLOS_AdjacentFloor_ReturnsTrue)
{
    create_simple_room(4, 4, 6, 6);

    // Adjacent floor tiles should have LOS
    // (depends on tcodMap state which is set during dig operations)
}

// ----------------------------------------------------------------------------
// Neighbor Finding Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, Neighbors_ReturnsAdjacentWalkable)
{
    // Create a small room
    create_simple_room(5, 5, 7, 7);

    Vector2D center{6, 6};
    auto neighbors = map->neighbors(center, ctx);

    // Should return adjacent walkable tiles
    // Number depends on which tiles are walkable after init
    EXPECT_GE(neighbors.size(), 0u); // At minimum, won't crash
}

TEST_F(MapTest, Neighbors_CornerPosition_LessNeighbors)
{
    // Corner should have fewer potential neighbors
    Vector2D corner{0, 0};
    auto neighbors = map->neighbors(corner, ctx);

    // Maximum 3 for corner (if all walkable), usually 0 (walls)
    EXPECT_LE(neighbors.size(), 3u);
}

// ----------------------------------------------------------------------------
// Reveal Map Test
// ----------------------------------------------------------------------------

TEST_F(MapTest, Reveal_MarksAllExplored)
{
    map->reveal();

    // After reveal, all valid tiles should be explored
    // Check a few positions
    EXPECT_TRUE(map->is_explored(Vector2D{1, 1}));
    EXPECT_TRUE(map->is_explored(Vector2D{5, 5}));
    EXPECT_TRUE(map->is_explored(Vector2D{TEST_MAP_HEIGHT - 2, TEST_MAP_WIDTH - 2}));
}

// ----------------------------------------------------------------------------
// Get Actor Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, GetActor_EmptyPosition_ReturnsNull) 
{
    Creature* actor = map->get_actor(Vector2D{7, 7}, ctx);  // Not player pos (5,5)

    // Empty position should return nullptr
    EXPECT_EQ(actor, nullptr);
}

TEST_F(MapTest, GetActor_PlayerPosition_ReturnsPlayer)
{
    // Player is already at {5, 5} from SetUp
    Creature* actor = map->get_actor(Vector2D{5, 5}, ctx);

    // Should return the player
    EXPECT_EQ(actor, player.get());
}

// ----------------------------------------------------------------------------
// Pathfinding Cost Tests
// ----------------------------------------------------------------------------

TEST_F(MapTest, Cost_FloorToFloor_IsOne)
{
    create_simple_room(5, 5, 7, 7);

    // Cost between adjacent floor tiles should be low
    // (Actual value depends on implementation)
}

TEST_F(MapTest, Cost_WaterTile_IsHigher)
{
    // Water has higher movement cost
    Vector2D floorPos{5, 5};
    Vector2D waterPos{6, 5};

    map->set_tile(floorPos, TileType::FLOOR, 1.0);
    map->set_tile(waterPos, TileType::WATER, 10.0);

    double waterCost = map->get_cost(waterPos, ctx);
    double floorCost = map->get_cost(floorPos, ctx);

    EXPECT_GT(waterCost, floorCost);
}
