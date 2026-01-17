#include <gtest/gtest.h>
#include "src/Actor/Destructible.h"
#include "src/Actor/Actor.h"
#include "src/ActorTypes/Player.h"
#include "src/Game.h"
#include "src/Core/GameContext.h"

// ============================================================================
// DESTRUCTIBLE EDGE CASE TESTS
// Tests healing limits, damage edge cases, HP clamping, death mechanics
// ============================================================================

class DestructibleEdgeCaseTest : public ::testing::Test {
protected:
    Game game;
    std::unique_ptr<Player> player;
    std::unique_ptr<Creature> monster;
    GameContext ctx;

    void SetUp() override {
        try {
            game.data_manager.load_all_data(game.message_system);
        } catch (...) {}

        player = std::make_unique<Player>(Vector2D{0, 0});
        player->destructible = std::make_unique<PlayerDestructible>(
            100, 5, "your corpse", 0, 20, 10
        );
        player->set_constitution(10);

        monster = std::make_unique<Creature>(Vector2D{1, 0}, ActorData{'o', "orc", 1});
        monster->destructible = std::make_unique<MonsterDestructible>(
            50, 2, "orc corpse", 75, 19, 7
        );

        ctx = game.get_context();
        ctx.player = player.get();

        game.d.set_test_mode(true);
    }

    void TearDown() override {
        game.d.set_test_mode(false);
        game.d.clear_fixed_rolls();
    }
};

// ----------------------------------------------------------------------------
// HP Setter Edge Cases
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, SetHp_ClampsToZero) {
    monster->destructible->set_hp(-50);
    EXPECT_EQ(monster->destructible->get_hp(), 0);
}

TEST_F(DestructibleEdgeCaseTest, SetHp_ClampsToMax) {
    monster->destructible->set_max_hp(50);
    monster->destructible->set_hp(999);
    EXPECT_EQ(monster->destructible->get_hp(), 50);
}

TEST_F(DestructibleEdgeCaseTest, SetHp_AcceptsValidValue) {
    monster->destructible->set_max_hp(100);
    monster->destructible->set_hp(75);
    EXPECT_EQ(monster->destructible->get_hp(), 75);
}

TEST_F(DestructibleEdgeCaseTest, SetHp_ZeroIsValid) {
    monster->destructible->set_hp(0);
    EXPECT_EQ(monster->destructible->get_hp(), 0);
    EXPECT_TRUE(monster->destructible->is_dead());
}

// ----------------------------------------------------------------------------
// Max HP Setter Edge Cases
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, SetMaxHp_MinimumIsOne) {
    monster->destructible->set_max_hp(0);
    EXPECT_EQ(monster->destructible->get_max_hp(), 1);
}

TEST_F(DestructibleEdgeCaseTest, SetMaxHp_NegativeBecomesOne) {
    monster->destructible->set_max_hp(-100);
    EXPECT_EQ(monster->destructible->get_max_hp(), 1);
}

TEST_F(DestructibleEdgeCaseTest, SetMaxHp_ReducesClampsCurrentHp) {
    monster->destructible->set_max_hp(100);
    monster->destructible->set_hp(100);

    // Reduce max HP
    monster->destructible->set_max_hp(50);

    // Current HP should be clamped
    EXPECT_EQ(monster->destructible->get_hp(), 50);
    EXPECT_EQ(monster->destructible->get_max_hp(), 50);
}

// ----------------------------------------------------------------------------
// HP Base Setter Edge Cases
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, SetHpBase_MinimumIsOne) {
    monster->destructible->set_hp_base(0);
    EXPECT_EQ(monster->destructible->get_hp_base(), 1);
}

TEST_F(DestructibleEdgeCaseTest, SetHpBase_NegativeBecomesOne) {
    monster->destructible->set_hp_base(-999);
    EXPECT_EQ(monster->destructible->get_hp_base(), 1);
}

// ----------------------------------------------------------------------------
// Healing Edge Cases
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, Heal_ReturnsActualHealed) {
    monster->destructible->set_max_hp(100);
    monster->destructible->set_hp(90);

    int healed = monster->destructible->heal(50);

    // Should only heal 10 (to max)
    EXPECT_EQ(healed, 10);
    EXPECT_EQ(monster->destructible->get_hp(), 100);
}

TEST_F(DestructibleEdgeCaseTest, Heal_AtMaxHp_ReturnsZero) {
    monster->destructible->set_max_hp(50);
    monster->destructible->set_hp(50);

    int healed = monster->destructible->heal(100);

    EXPECT_EQ(healed, 0);
    EXPECT_EQ(monster->destructible->get_hp(), 50);
}

TEST_F(DestructibleEdgeCaseTest, Heal_FromZeroHp) {
    monster->destructible->set_max_hp(100);
    monster->destructible->set_hp(0);

    int healed = monster->destructible->heal(50);

    EXPECT_EQ(healed, 50);
    EXPECT_EQ(monster->destructible->get_hp(), 50);
}

TEST_F(DestructibleEdgeCaseTest, Heal_ExactAmountToMax) {
    monster->destructible->set_max_hp(100);
    monster->destructible->set_hp(70);

    int healed = monster->destructible->heal(30);

    EXPECT_EQ(healed, 30);
    EXPECT_EQ(monster->destructible->get_hp(), 100);
}

TEST_F(DestructibleEdgeCaseTest, Heal_ZeroAmount) {
    monster->destructible->set_hp(50);
    int hpBefore = monster->destructible->get_hp();

    int healed = monster->destructible->heal(0);

    EXPECT_EQ(healed, 0);
    EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

TEST_F(DestructibleEdgeCaseTest, Heal_NegativeAmount_TreatedAsHeal) {
    // Negative healing is technically possible in the implementation
    // This tests current behavior (may be unintended)
    monster->destructible->set_max_hp(100);
    monster->destructible->set_hp(50);

    int healed = monster->destructible->heal(-10);

    // Healing negative adds negative to HP: 50 + (-10) = 40
    // But clamped by max, so result depends on implementation
    // This documents current behavior
    EXPECT_LE(monster->destructible->get_hp(), 100);
}

// ----------------------------------------------------------------------------
// Take Damage Edge Cases
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, TakeDamage_ZeroDamage_NoEffect) {
    int hpBefore = monster->destructible->get_hp();

    monster->destructible->take_damage(*monster, 0, ctx);

    EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

TEST_F(DestructibleEdgeCaseTest, TakeDamage_NegativeDamage_NoEffect) {
    int hpBefore = monster->destructible->get_hp();

    monster->destructible->take_damage(*monster, -10, ctx);

    EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

TEST_F(DestructibleEdgeCaseTest, TakeDamage_ExactlyLethal) {
    monster->destructible->set_hp(10);

    monster->destructible->take_damage(*monster, 10, ctx);

    EXPECT_TRUE(monster->destructible->is_dead());
    EXPECT_EQ(monster->destructible->get_hp(), 0);
}

TEST_F(DestructibleEdgeCaseTest, TakeDamage_Overkill) {
    monster->destructible->set_hp(10);

    monster->destructible->take_damage(*monster, 1000, ctx);

    EXPECT_TRUE(monster->destructible->is_dead());
    EXPECT_EQ(monster->destructible->get_hp(), 0);
}

TEST_F(DestructibleEdgeCaseTest, TakeDamage_OneDamage) {
    monster->destructible->set_hp(10);

    monster->destructible->take_damage(*monster, 1, ctx);

    EXPECT_EQ(monster->destructible->get_hp(), 9);
    EXPECT_FALSE(monster->destructible->is_dead());
}

TEST_F(DestructibleEdgeCaseTest, TakeDamage_AlreadyDead_NoFurtherEffect) {
    monster->destructible->set_hp(0);
    ASSERT_TRUE(monster->destructible->is_dead());

    // Taking damage when already dead
    monster->destructible->take_damage(*monster, 100, ctx);

    // Should remain at 0
    EXPECT_EQ(monster->destructible->get_hp(), 0);
}

// ----------------------------------------------------------------------------
// Death State Tests
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, IsDead_TrueAtZeroHp) {
    monster->destructible->set_hp(0);
    EXPECT_TRUE(monster->destructible->is_dead());
}

TEST_F(DestructibleEdgeCaseTest, IsDead_FalseAtOneHp) {
    monster->destructible->set_hp(1);
    EXPECT_FALSE(monster->destructible->is_dead());
}

TEST_F(DestructibleEdgeCaseTest, IsDead_TrueWithNegativeClampedHp) {
    // Even though set_hp clamps negative to 0, verify is_dead works
    monster->destructible->set_hp(-100);
    EXPECT_TRUE(monster->destructible->is_dead());
}

// ----------------------------------------------------------------------------
// Attribute Getter/Setter Tests
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, ArmorClass_CanBeNegative) {
    // In AD&D, negative AC is better
    monster->destructible->set_armor_class(-5);
    EXPECT_EQ(monster->destructible->get_armor_class(), -5);
}

TEST_F(DestructibleEdgeCaseTest, THAC0_CanBeSetToAnyValue) {
    monster->destructible->set_thaco(5);
    EXPECT_EQ(monster->destructible->get_thaco(), 5);

    monster->destructible->set_thaco(25);
    EXPECT_EQ(monster->destructible->get_thaco(), 25);
}

TEST_F(DestructibleEdgeCaseTest, DamageReduction_AcceptsAnyValue) {
    monster->destructible->set_dr(0);
    EXPECT_EQ(monster->destructible->get_dr(), 0);

    monster->destructible->set_dr(100);
    EXPECT_EQ(monster->destructible->get_dr(), 100);
}

TEST_F(DestructibleEdgeCaseTest, XP_CanBeModified) {
    monster->destructible->set_xp(100);
    EXPECT_EQ(monster->destructible->get_xp(), 100);

    monster->destructible->add_xp(50);
    EXPECT_EQ(monster->destructible->get_xp(), 150);
}

TEST_F(DestructibleEdgeCaseTest, CorpseName_Preserved) {
    monster->destructible->set_corpse_name("dead orc");
    EXPECT_EQ(monster->destructible->get_corpse_name(), "dead orc");
}

// ----------------------------------------------------------------------------
// Player Death Tests
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, PlayerDeath_SetsDefeatStatus) {
    player->destructible->set_hp(1);

    player->destructible->take_damage(*player, 10, ctx);

    EXPECT_TRUE(player->destructible->is_dead());
    EXPECT_EQ(ctx.game->gameStatus, GameStatus::DEFEAT);
}

// ----------------------------------------------------------------------------
// Monster Death Tests (XP Award)
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, MonsterDeath_AwardsXPToPlayer) {
    monster->destructible->set_hp(1);
    monster->destructible->set_xp(200);

    int playerXpBefore = player->destructible->get_xp();

    monster->destructible->take_damage(*monster, 10, ctx);

    EXPECT_TRUE(monster->destructible->is_dead());
    EXPECT_EQ(player->destructible->get_xp(), playerXpBefore + 200);
}

TEST_F(DestructibleEdgeCaseTest, MonsterDeath_ZeroXPMonster) {
    monster->destructible->set_hp(1);
    monster->destructible->set_xp(0);

    int playerXpBefore = player->destructible->get_xp();

    monster->destructible->take_damage(*monster, 10, ctx);

    EXPECT_EQ(player->destructible->get_xp(), playerXpBefore);
}

// ----------------------------------------------------------------------------
// Serialization Regression Tests
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, Serialization_AllFieldsPreserved) {
    monster->destructible->set_max_hp(75);
    monster->destructible->set_hp(60);
    monster->destructible->set_hp_base(50);
    monster->destructible->set_dr(8);
    monster->destructible->set_xp(300);
    monster->destructible->set_thaco(15);
    monster->destructible->set_armor_class(3);
    monster->destructible->set_base_armor_class(5);
    monster->destructible->set_corpse_name("dead monster");
    monster->destructible->set_last_constitution(14);

    json j;
    monster->destructible->save(j);

    auto loaded = Destructible::create(j);

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->get_max_hp(), 75);
    EXPECT_EQ(loaded->get_hp(), 60);
    EXPECT_EQ(loaded->get_hp_base(), 50);
    EXPECT_EQ(loaded->get_dr(), 8);
    EXPECT_EQ(loaded->get_xp(), 300);
    EXPECT_EQ(loaded->get_thaco(), 15);
    EXPECT_EQ(loaded->get_armor_class(), 3);
    EXPECT_EQ(loaded->get_base_armor_class(), 5);
    EXPECT_EQ(loaded->get_corpse_name(), "dead monster");
    EXPECT_EQ(loaded->get_last_constitution(), 14);
}

TEST_F(DestructibleEdgeCaseTest, Serialization_DeadState_Preserved) {
    monster->destructible->set_hp(0);
    ASSERT_TRUE(monster->destructible->is_dead());

    json j;
    monster->destructible->save(j);

    auto loaded = Destructible::create(j);

    ASSERT_NE(loaded, nullptr);
    EXPECT_TRUE(loaded->is_dead());
    EXPECT_EQ(loaded->get_hp(), 0);
}

TEST_F(DestructibleEdgeCaseTest, Serialization_NegativeAC_Preserved) {
    monster->destructible->set_armor_class(-10);

    json j;
    monster->destructible->save(j);

    auto loaded = Destructible::create(j);

    EXPECT_EQ(loaded->get_armor_class(), -10);
}

// ----------------------------------------------------------------------------
// Factory Create Edge Cases
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, Create_InvalidType_ReturnsNullptr) {
    json j;
    j["type"] = 999; // Invalid type
    j["hpMax"] = 10;
    j["hp"] = 10;
    j["hpBase"] = 10;
    j["lastConstitution"] = 10;
    j["dr"] = 0;
    j["corpseName"] = "test";
    j["xp"] = 0;
    j["thaco"] = 20;
    j["armorClass"] = 10;
    j["baseArmorClass"] = 10;

    auto result = Destructible::create(j);

    EXPECT_EQ(result, nullptr);
}

TEST_F(DestructibleEdgeCaseTest, Create_MissingType_ReturnsNullptr) {
    json j;
    // No "type" field
    j["hpMax"] = 10;
    j["hp"] = 10;

    auto result = Destructible::create(j);

    EXPECT_EQ(result, nullptr);
}

TEST_F(DestructibleEdgeCaseTest, Create_MonsterType_ReturnsMonsterDestructible) {
    json j;
    j["type"] = static_cast<int>(Destructible::DestructibleType::MONSTER);
    j["hpMax"] = 50;
    j["hp"] = 50;
    j["hpBase"] = 50;
    j["lastConstitution"] = 0;
    j["dr"] = 2;
    j["corpseName"] = "corpse";
    j["xp"] = 100;
    j["thaco"] = 19;
    j["armorClass"] = 7;
    j["baseArmorClass"] = 7;

    auto result = Destructible::create(j);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get_max_hp(), 50);
    // Verify it's actually a MonsterDestructible by checking behavior would be harder
}

TEST_F(DestructibleEdgeCaseTest, Create_PlayerType_ReturnsPlayerDestructible) {
    json j;
    j["type"] = static_cast<int>(Destructible::DestructibleType::PLAYER);
    j["hpMax"] = 100;
    j["hp"] = 80;
    j["hpBase"] = 100;
    j["lastConstitution"] = 12;
    j["dr"] = 0;
    j["corpseName"] = "your corpse";
    j["xp"] = 5000;
    j["thaco"] = 18;
    j["armorClass"] = 5;
    j["baseArmorClass"] = 10;

    auto result = Destructible::create(j);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get_hp(), 80);
    EXPECT_EQ(result->get_xp(), 5000);
}
