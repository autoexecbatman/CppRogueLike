#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "src/Actor/Actor.h"
#include "src/Actor/Destructible.h"
#include "src/Actor/Attacker.h"
#include "src/Ai/AiMonster.h"

using json = nlohmann::json;

// ============================================================================
// CREATURE SERIALIZATION TESTS
// Ensures creatures save/load with all components intact
// ============================================================================

class CreatureSerializationTest : public ::testing::Test {
protected:
    std::unique_ptr<Creature> create_test_creature() {
        auto creature = std::make_unique<Creature>(
            Vector2D{ 20, 10 },
            ActorData{'g', "goblin", 1}
        );
        creature->set_strength(14);
        creature->set_dexterity(12);
        creature->set_constitution(10);
        creature->set_intelligence(8);
        creature->set_wisdom(7);
        creature->set_charisma(6);
        creature->set_gold(50);
        creature->set_weapon_equipped("Short Sword");

        // Add components - use MonsterDestructible for proper serialization
        creature->destructible = std::make_unique<MonsterDestructible>(20, 1, "dead goblin", 35, 19, 6);
        creature->attacker = std::make_unique<Attacker>(DamageInfo{1, 6, "1d6"});
        creature->ai = std::make_unique<AiMonster>();

        return creature;
    }
};

TEST_F(CreatureSerializationTest, FullCreature_SaveLoad_RoundTrip) {
    auto original = create_test_creature();

    // Save
    json j;
    original->save(j);

    // Load into new creature
    auto loaded = std::make_unique<Creature>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    // Verify position (Vector2D is {y, x})
    EXPECT_EQ(loaded->position.x, 20);
    EXPECT_EQ(loaded->position.y, 10);

    // Verify stats
    EXPECT_EQ(loaded->get_strength(), 14);
    EXPECT_EQ(loaded->get_dexterity(), 12);
    EXPECT_EQ(loaded->get_constitution(), 10);
    EXPECT_EQ(loaded->get_gold(), 50);
    EXPECT_EQ(loaded->get_weapon_equipped(), "Short Sword");

    // Verify components exist
    ASSERT_NE(loaded->destructible, nullptr) << "Destructible not loaded";
    ASSERT_NE(loaded->attacker, nullptr) << "Attacker not loaded";
    ASSERT_NE(loaded->ai, nullptr) << "AI not loaded";

    // Verify destructible values
    EXPECT_EQ(loaded->destructible->get_max_hp(), 20);
    EXPECT_EQ(loaded->destructible->get_dr(), 1);
    EXPECT_EQ(loaded->destructible->get_xp(), 35);
}

TEST_F(CreatureSerializationTest, Creature_WithDamage_PreserveHP) {
    auto original = create_test_creature();
    original->destructible->set_hp(15); // HP: 20 -> 15

    json j;
    original->save(j);

    auto loaded = std::make_unique<Creature>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    ASSERT_NE(loaded->destructible, nullptr);
    EXPECT_EQ(loaded->destructible->get_hp(), 15);
}

TEST_F(CreatureSerializationTest, Creature_Dead_PreservesState) {
    auto original = create_test_creature();
    original->destructible->set_hp(-5); // Kill it

    ASSERT_TRUE(original->destructible->is_dead());

    json j;
    original->save(j);

    auto loaded = std::make_unique<Creature>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    ASSERT_NE(loaded->destructible, nullptr);
    EXPECT_TRUE(loaded->destructible->is_dead());
}

TEST_F(CreatureSerializationTest, Creature_NoDestructible_HandledGracefully) {
    // Create creature without destructible
    auto original = std::make_unique<Creature>(Vector2D{5, 5}, ActorData{'?', "mystery", 1});
    // Don't add destructible

    json j;
    original->save(j);

    // Verify destructible section not in JSON
    EXPECT_FALSE(j.contains("destructible"));

    auto loaded = std::make_unique<Creature>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    // Should be null, not crash
    EXPECT_EQ(loaded->destructible, nullptr);
}

TEST_F(CreatureSerializationTest, AttackerDamage_Preserved) {
    auto original = create_test_creature();

    json j;
    original->save(j);

    auto loaded = std::make_unique<Creature>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    ASSERT_NE(loaded->attacker, nullptr);

    // Get damage info and verify
    auto damageInfo = loaded->attacker->get_attack_damage(*loaded);
    EXPECT_EQ(damageInfo.minDamage, 1);
    EXPECT_EQ(damageInfo.maxDamage, 6);
}
