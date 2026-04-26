#include <gtest/gtest.h>
#include <memory>
#include <nlohmann/json.hpp>
#include "src/Actor/Destructible.h"

using json = nlohmann::json;

// ============================================================================
// DESTRUCTIBLE SERIALIZATION TESTS
// Ensures save/load round-trip works correctly - prevents shopkeeper-style bugs
// Note: Death handler type (PLAYER/MONSTER) is now determined by Creature class,
// not by Destructible. Destructible only manages HealthPool and ConstitutionTracker.
// ============================================================================

class DestructibleSerializationTest : public ::testing::Test {
protected:
    // Standard test values
    static constexpr int HP_MAX = 50;
};

// HP and constitution state round-trip
TEST_F(DestructibleSerializationTest, Destructible_SaveLoad_RoundTrip) {
    // Create with new constructor signature
    Destructible original(HP_MAX);
    // Directly set HP to simulate damage (take_damage requires GameContext)
    original.set_hp(40);

    // Save
    json j;
    original.save(j);

    // Verify required fields are saved
    ASSERT_TRUE(j.contains("hpMax")) << "Destructible must save 'hpMax' field";
    ASSERT_TRUE(j.contains("hp")) << "Destructible must save 'hp' field";
    ASSERT_TRUE(j.contains("lastConstitution")) << "Destructible must save 'lastConstitution' field";

    // Load via factory
    auto loaded = Destructible::create(j);

    // Verify not null
    ASSERT_NE(loaded, nullptr) << "Destructible::create returned nullptr";

    // Verify HP values
    EXPECT_EQ(loaded->get_max_hp(), HP_MAX);
    EXPECT_EQ(loaded->get_hp(), 40); // Damaged
}

// Full HP state preservation
TEST_F(DestructibleSerializationTest, FullHP_PreservedAfterLoad) {
    Destructible original(HP_MAX);
    // No damage - full HP

    json j;
    original.save(j);

    auto loaded = Destructible::create(j);

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->get_hp(), HP_MAX);
    EXPECT_EQ(loaded->get_max_hp(), HP_MAX);
}

// Death state persists
TEST_F(DestructibleSerializationTest, DeathState_Persists) {
    Destructible original(10);
    original.set_hp(0); // Mark as dead

    ASSERT_TRUE(original.is_dead());

    json j;
    original.save(j);

    auto loaded = Destructible::create(j);
    ASSERT_NE(loaded, nullptr);
    EXPECT_TRUE(loaded->is_dead()) << "Death state not preserved after load";
    EXPECT_EQ(loaded->get_hp(), 0);
}

// Temp HP preservation
TEST_F(DestructibleSerializationTest, TempHP_PreservedAfterLoad) {
    Destructible original(50);
    original.set_hp(40);
    original.set_temp_hp(20);

    json j;
    original.save(j);

    auto loaded = Destructible::create(j);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->get_temp_hp(), 20);
    EXPECT_EQ(loaded->get_effective_hp(), 60); // 40 + 20
}

// HP Base preservation (for level-up scenarios)
TEST_F(DestructibleSerializationTest, HPBase_PreservedAfterLoad) {
    Destructible original(50);
    original.set_hp_base(55); // Simulates level-up HP increase
    original.set_hp(55);

    json j;
    original.save(j);

    auto loaded = Destructible::create(j);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->get_hp_base(), 55);
}
