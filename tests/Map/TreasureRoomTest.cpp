#include <gtest/gtest.h>
#include <set>
#include <string>

#include "src/Actor/Object.h"
#include "src/Factories/ItemCreator.h"
#include "src/Factories/MonsterCreator.h"
#include "src/Map/DungeonNames.h"
#include "src/Map/DungeonRoom.h"
#include "src/Map/Map.h"
#include "src/Actor/Destructible.h"
#include "src/Actor/Stairs.h"
#include "src/ActorTypes/Player.h"
#include "src/Combat/ExperienceReward.h"
#include "src/Core/GameContext.h"
#include "src/Core/Paths.h"
#include "src/Random/RandomDice.h"
#include "src/Systems/DataManager.h"
#include "src/Systems/LevelManager.h"
#include "src/Systems/MessageSystem.h"
#include "tests/mocks/MockGameContext.h"

// ============================================================================
// Treasure Room Tests
// Contract:
//   - generate_warden_name produces valid, varied names
//   - count_room_entrances counts only genuine room-entrance doors
//   - After map init, the staircase tile is never adjacent to a locked door
//   - If setup_treasure_room_guard cannot place a jailer, all doors it locked
//     in Pass 1 are rolled back to CLOSED_UNLOCKED
// ============================================================================

namespace
{
    // Fixture map dimensions — single TU, no reason to be class members.
    constexpr int FIXTURE_W = 40;
    constexpr int FIXTURE_H = 25;

    // Integration test map dimensions.
    constexpr int STAIR_TEST_W = 120;
    constexpr int STAIR_TEST_H = 80;
}

// Exposes the protected setup_treasure_room_guard so tests can call it directly.
class TestableTreasureMap : public Map
{
public:
    TestableTreasureMap(int w, int h) : Map(w, h) {}
    using Map::setup_treasure_room_guard;
};

// ----------------------------------------------------------------------------
// Shared fixture
// ----------------------------------------------------------------------------

class TreasureRoomFixture : public ::testing::Test
{
protected:
    std::unique_ptr<TestableTreasureMap> map;
    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Creature>> creatures;
    MockGameContext mock;
    GameContext ctx;
    DataManager dataManager;
    MessageSystem messageSystem;

    void SetUp() override
    {
        try
        {
            dataManager.load_all_data(messageSystem);
            ItemCreator::load(Paths::ITEMS);
            ItemCreator::load_enhanced_rules(Paths::ENHANCED_RULES);
            MonsterCreator::load(Paths::MONSTERS);
        }
        catch (...) {}

        map = std::make_unique<TestableTreasureMap>(FIXTURE_W, FIXTURE_H);
        player = std::make_unique<Player>(Vector2D{ 1, 1 });
        player->experienceReward = std::make_unique<ExperienceReward>(0);
        player->destructible = std::make_unique<Destructible>(
            20, 0, "your corpse", 0, 20, 10, std::make_unique<PlayerDeathHandler>());

        // Use MockGameContext as base so contentRegistry, tileConfig, and
        // inventoryData are all wired — setup_treasure_room_guard needs
        // contentRegistry to create the dungeon_key item for the jailer.
        ctx = mock.to_game_context();
        ctx.player = player.get();
        ctx.dataManager = &dataManager;
        ctx.messageSystem = &messageSystem;
        ctx.creatures = &creatures;
        ctx.map = map.get();

        map->init_tiles();  // blank wall grid; tests manually stamp rooms and doors
    }

    // Stamp a room floor and return its descriptor.
    DungeonRoom make_room(int col, int row, int w, int h)
    {
        for (int y = row; y < row + h; ++y)
        {
            for (int x = col; x < col + w; ++x)
            {
                map->set_tile(Vector2D{ x, y }, TileType::FLOOR, 1.0);
            }
        }
        return DungeonRoom{ col, row, w, h };
    }

    void place_door(Vector2D pos)
    {
        map->set_tile(pos, TileType::CLOSED_DOOR, 2.0);
    }

    void place_corridor(Vector2D pos)
    {
        map->set_tile(pos, TileType::CORRIDOR, 1.0);
    }
};

// ============================================================================
// DungeonNames::generate_warden_name
// ============================================================================

TEST(DungeonNames, ReturnsNonEmptyString)
{
    RandomDice rng{ 42 };
    EXPECT_FALSE(DungeonNames::generate_warden_name(rng).empty());
}

TEST(DungeonNames, NameContainsSpace)
{
    // Format is "[FirstName] [Epithet]" — always has at least one space.
    RandomDice rng{ 42 };
    const std::string name = DungeonNames::generate_warden_name(rng);
    EXPECT_NE(name.find(' '), std::string::npos)
        << "Expected space between name and epithet, got: " << name;
}

TEST(DungeonNames, NeverEmptyAcross50Seeds)
{
    for (unsigned int seed = 0; seed < 50; ++seed)
    {
        RandomDice rng{ seed };
        EXPECT_FALSE(DungeonNames::generate_warden_name(rng).empty())
            << "Empty name at seed " << seed;
    }
}

TEST(DungeonNames, ProducesVariedNamesAcrossSeeds)
{
    // 225 combinations — 20 seeds must yield more than 1 distinct result.
    // P(all 20 identical) < (1/225)^19 — effectively impossible.
    std::set<std::string> seen;
    for (unsigned int seed = 0; seed < 20; ++seed)
    {
        RandomDice rng{ seed };
        seen.insert(DungeonNames::generate_warden_name(rng));
    }
    EXPECT_GT(seen.size(), 1u)
        << "All 20 seeds produced the same name — pool is not being sampled";
}

// ============================================================================
// Map::count_room_entrances
// ============================================================================

TEST_F(TreasureRoomFixture, CountEntrances_NoDoors_ReturnsZero)
{
    const DungeonRoom room = make_room(5, 5, 6, 6);
    EXPECT_EQ(map->count_room_entrances(room), 0);
}

TEST_F(TreasureRoomFixture, CountEntrances_OneLeftWallDoor_ReturnsOne)
{
    // Floor [5..10] x [5..10].  Door at (4,7).
    // (4,7) + DIR_E = (5,7) — inside room.contains() — genuine entrance.
    const DungeonRoom room = make_room(5, 5, 6, 6);
    place_door(Vector2D{ 4, 7 });
    EXPECT_EQ(map->count_room_entrances(room), 1);
}

TEST_F(TreasureRoomFixture, CountEntrances_OneTopWallDoor_ReturnsOne)
{
    const DungeonRoom room = make_room(5, 5, 6, 6);
    place_door(Vector2D{ 7, 4 });
    EXPECT_EQ(map->count_room_entrances(room), 1);
}

TEST_F(TreasureRoomFixture, CountEntrances_TwoDoors_ReturnsTwo)
{
    const DungeonRoom room = make_room(5, 5, 6, 6);
    place_door(Vector2D{ 4, 7 });  // left wall
    place_door(Vector2D{ 7, 4 });  // top wall
    EXPECT_EQ(map->count_room_entrances(room), 2);
}

TEST_F(TreasureRoomFixture, CountEntrances_FarDoor_NotCounted)
{
    // A door with no room-interior cardinal neighbour is not an entrance.
    const DungeonRoom room = make_room(5, 5, 6, 6);
    place_door(Vector2D{ 20, 15 });
    EXPECT_EQ(map->count_room_entrances(room), 0);
}

// ============================================================================
// setup_treasure_room_guard rollback
// When the outward walk from the door is immediately blocked by a wall,
// candidates stays empty and the function must unlock all doors Pass 1 locked.
// ============================================================================

TEST_F(TreasureRoomFixture, GuardSetup_NoJailerSpawn_DoorsRolledBack)
{
    // Room floor at [10..15] x [5..10].
    // Door on left wall at (9, 7).
    // The tile at (8, 7) stays WALL — outward walk blocked at step 1.
    // -> candidates is empty -> unlock_all_room_entrances must fire.

    const DungeonRoom room = make_room(10, 5, 6, 6);
    const Vector2D doorPos{ 9, 7 };
    place_door(doorPos);

    // The dungeon generator in SetUp may have placed floor/corridor at (8,7).
    // Explicitly reset it to WALL to guarantee the outward walk is blocked.
    map->set_tile(Vector2D{ 8, 7 }, TileType::WALL, 0.0);

    // Confirm the outward tile is a wall (precondition).
    ASSERT_EQ(map->get_tile_type(Vector2D{ 8, 7 }), TileType::WALL);

    map->setup_treasure_room_guard(room, ctx);

    // The door must NOT remain locked after the failed guard setup.
    EXPECT_FALSE(map->is_door_locked(doorPos))
        << "Door at " << doorPos.x << "," << doorPos.y
        << " must be unlocked when no jailer can be placed";
}

TEST_F(TreasureRoomFixture, GuardSetup_WithCorridor_JailerIsPlaced)
{
    // Room floor at [10..15] x [5..10].
    // Door on left wall at (9, 7).
    // Corridor tile at (8, 7) — outward walk succeeds at step 1.
    // -> jailer spawned at (8, 7) -> door remains locked.

    const DungeonRoom room = make_room(10, 5, 6, 6);
    const Vector2D doorPos{ 9, 7 };
    const Vector2D corridorPos{ 8, 7 };
    place_door(doorPos);
    place_corridor(corridorPos);

    const std::size_t creaturesBefore = creatures.size();
    map->setup_treasure_room_guard(room, ctx);

    // Door must remain locked — a jailer guards it.
    EXPECT_TRUE(map->is_door_locked(doorPos))
        << "Door should stay locked when jailer is successfully placed";

    // A creature (the jailer) must have been added.
    EXPECT_GT(creatures.size(), creaturesBefore)
        << "Expected jailer to be spawned in the corridor";
}

// ============================================================================
// Staircase room never locked (integration)
// After map init, no locked door should be adjacent to the stairs tile.
//
// Uses MockGameContext to supply tileConfig, contentRegistry, and inventoryData.
// Full init (stairs, rooms, levelManager, creatures all wired in the test body).
// ============================================================================

TEST(StairRoomNotLocked, StairsAreNeverAdjacentToLockedDoor)
{
    // Content registries are global singletons — load once.
    try
    {
        ItemCreator::load(Paths::ITEMS);
        ItemCreator::load_enhanced_rules(Paths::ENHANCED_RULES);
        MonsterCreator::load(Paths::MONSTERS);
    }
    catch (...) {}

    MockGameContext mock;

    auto map = std::make_unique<Map>(STAIR_TEST_W, STAIR_TEST_H);
    auto player = std::make_unique<Player>(Vector2D{ 5, 5 });
    player->experienceReward = std::make_unique<ExperienceReward>(0);
    player->destructible = std::make_unique<Destructible>(
        20, 0, "your corpse", 0, 20, 10, std::make_unique<PlayerDeathHandler>());
    auto stairs = std::make_unique<Stairs>(Vector2D{ 0, 0 });

    std::vector<std::unique_ptr<Creature>> creatures;
    std::vector<std::unique_ptr<Object>> objects;
    std::vector<DungeonRoom> rooms;
    DataManager dataManager;
    MessageSystem messageSystem;
    LevelManager levelManager;

    try { dataManager.load_all_data(messageSystem); } catch (...) {}

    // Start from the mock's wired context (tileConfig, contentRegistry,
    // inventoryData, dice all set), then override the test-specific members.
    GameContext ctx = mock.to_game_context();
    ctx.player = player.get();
    ctx.stairs = stairs.get();
    ctx.rooms = &rooms;
    ctx.creatures = &creatures;
    ctx.objects = &objects;
    ctx.dataManager = &dataManager;
    ctx.messageSystem = &messageSystem;
    ctx.map = map.get();
    ctx.levelManager = &levelManager;

    // Full init: ctx wires stairs, rooms, levelManager, dice, and the mock provides
    // tileConfig, contentRegistry, inventoryData. Verifies the stair-adjacency
    // invariant after a complete dungeon generation pass.
    map->init(ctx);

    const Vector2D stairsPos = stairs->position;
    ASSERT_TRUE(map->is_in_bounds(stairsPos))
        << "Stairs at (" << stairsPos.x << "," << stairsPos.y << ") out of bounds";

    for (const Vector2D dir : { DIR_N, DIR_S, DIR_E, DIR_W })
    {
        const Vector2D neighbour = stairsPos + dir;
        if (!map->is_in_bounds(neighbour))
        {
            continue;
        }
        EXPECT_FALSE(map->is_door_locked(neighbour))
            << "Locked door at (" << neighbour.x << "," << neighbour.y
            << ") adjacent to stairs at (" << stairsPos.x << "," << stairsPos.y << ")";
    }
}
