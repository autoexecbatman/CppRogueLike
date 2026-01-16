#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "src/Actor/Destructible.h"

using json = nlohmann::json;

// ============================================================================
// DESTRUCTIBLE SERIALIZATION TESTS
// Ensures save/load round-trip works correctly - prevents shopkeeper-style bugs
// ============================================================================

class DestructibleSerializationTest : public ::testing::Test {
protected:
    // Standard test values
    static constexpr int HP_MAX = 50;
    static constexpr int DR = 5;
    static constexpr int XP = 100;
    static constexpr int THACO = 15;
    static constexpr int AC = 5;
    const std::string CORPSE_NAME = "test corpse";
};

// MonsterDestructible round-trip
TEST_F(DestructibleSerializationTest, MonsterDestructible_SaveLoad_RoundTrip) {
    // Create and configure
    MonsterDestructible original(HP_MAX, DR, CORPSE_NAME, XP, THACO, AC);
    // Directly set HP to simulate damage (take_damage requires GameContext)
    original.set_hp(40);

    // Save
    json j;
    original.save(j);

    // Verify type is saved
    ASSERT_TRUE(j.contains("type")) << "MonsterDestructible must save 'type' field";
    EXPECT_EQ(j["type"].get<int>(), static_cast<int>(Destructible::DestructibleType::MONSTER));

    // Load via factory
    auto loaded = Destructible::create(j);

    // Verify not null
    ASSERT_NE(loaded, nullptr) << "Destructible::create returned nullptr - type field missing or invalid";

    // Verify values
    EXPECT_EQ(loaded->get_max_hp(), HP_MAX);
    EXPECT_EQ(loaded->get_hp(), 40); // Damaged
    EXPECT_EQ(loaded->get_dr(), DR);
    EXPECT_EQ(loaded->get_corpse_name(), CORPSE_NAME);
    EXPECT_EQ(loaded->get_xp(), XP);
    EXPECT_EQ(loaded->get_thaco(), THACO);
    EXPECT_EQ(loaded->get_armor_class(), AC);
}

// PlayerDestructible round-trip
TEST_F(DestructibleSerializationTest, PlayerDestructible_SaveLoad_RoundTrip) {
    PlayerDestructible original(HP_MAX, DR, CORPSE_NAME, XP, THACO, AC);
    original.set_hp(30); // Simulate damage

    json j;
    original.save(j);

    // Verify type
    ASSERT_TRUE(j.contains("type"));
    EXPECT_EQ(j["type"].get<int>(), static_cast<int>(Destructible::DestructibleType::PLAYER));

    auto loaded = Destructible::create(j);

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->get_hp(), 30);
}

// Base Destructible should NOT be used directly (no type saved)
TEST_F(DestructibleSerializationTest, BaseDestructible_NoType_ReturnsNullptr) {
    // Simulate what happens if someone uses base Destructible
    json j;
    j["hpMax"] = HP_MAX;
    j["hp"] = HP_MAX;
    j["dr"] = DR;
    // NO "type" field

    auto loaded = Destructible::create(j);

    EXPECT_EQ(loaded, nullptr) << "Base Destructible without type should return nullptr on load";
}

// Invalid type returns nullptr
TEST_F(DestructibleSerializationTest, InvalidType_ReturnsNullptr) {
    json j;
    j["type"] = 999; // Invalid type
    j["hpMax"] = HP_MAX;

    auto loaded = Destructible::create(j);

    EXPECT_EQ(loaded, nullptr);
}

// Regression: Shopkeeper bug - ensure MonsterDestructible is distinguishable
TEST_F(DestructibleSerializationTest, Regression_ShopkeeperMustUseMonsterDestructible) {
    // The bug: ShopkeeperFactory used base Destructible which doesn't save type
    // This test ensures MonsterDestructible saves correctly

    MonsterDestructible shopkeeperDestructible(100, 20, "shopkeeper corpse", 0, 20, 10);

    json j;
    shopkeeperDestructible.save(j);

    // Must have type for proper loading
    ASSERT_TRUE(j.contains("type"));
    EXPECT_EQ(j["type"].get<int>(), static_cast<int>(Destructible::DestructibleType::MONSTER));

    // Must load back correctly
    auto loaded = Destructible::create(j);
    ASSERT_NE(loaded, nullptr) << "Shopkeeper destructible failed to load";
    EXPECT_EQ(loaded->get_max_hp(), 100);
}

// Death state persists
TEST_F(DestructibleSerializationTest, DeathState_Persists) {
    MonsterDestructible original(10, 0, CORPSE_NAME, XP, THACO, AC);
    original.set_hp(-5); // Kill it by setting HP below 0

    ASSERT_TRUE(original.is_dead());

    json j;
    original.save(j);

    auto loaded = Destructible::create(j);
    ASSERT_NE(loaded, nullptr);
    EXPECT_TRUE(loaded->is_dead()) << "Death state not preserved after load";
    EXPECT_LE(loaded->get_hp(), 0);
}
