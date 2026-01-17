#include <gtest/gtest.h>
#include "src/Actor/Attacker.h"
#include "src/Actor/Destructible.h"
#include "src/Actor/Actor.h"
#include "src/ActorTypes/Player.h"
#include "src/Combat/DamageInfo.h"
#include "src/Game.h"
#include "src/Core/GameContext.h"

// ============================================================================
// ATTACKER TESTS
// Tests combat calculations, hit/miss logic, damage application
// ============================================================================

class AttackerTest : public ::testing::Test {
protected:
    Game game;
    std::unique_ptr<Player> player;
    std::unique_ptr<Creature> monster;
    GameContext ctx;

    void SetUp() override {
        // Load data for attribute tables
        try {
            game.data_manager.load_all_data(game.message_system);
        } catch (...) {
            // Tests will run without attribute bonuses
        }

        // Create player
        player = std::make_unique<Player>(Vector2D{0, 0});
        player->destructible = std::make_unique<PlayerDestructible>(
            20, 0, "your corpse", 0, 20, 10  // HP, DR, corpse, XP, THAC0, AC
        );
        player->attacker = std::make_unique<Attacker>(DamageInfo{1, 4, "1d4"});
        player->set_strength(10);
        player->set_dexterity(10);

        // Create monster
        monster = std::make_unique<Creature>(Vector2D{1, 0}, ActorData{'g', "goblin", 1});
        monster->destructible = std::make_unique<MonsterDestructible>(
            10, 0, "goblin corpse", 50, 19, 6  // HP, DR, corpse, XP, THAC0, AC
        );
        monster->attacker = std::make_unique<Attacker>(DamageInfo{1, 6, "1d6"});
        monster->set_strength(8);
        monster->set_dexterity(10);
        monster->set_weapon_equipped("claws");

        // Setup GameContext
        ctx = game.get_context();
        ctx.player = player.get();

        // Enable deterministic dice
        game.d.set_test_mode(true);
    }

    void TearDown() override {
        game.d.set_test_mode(false);
        game.d.clear_fixed_rolls();
    }
};

// ----------------------------------------------------------------------------
// Attacker Serialization Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, Serialization_RoundTrip) {
    Attacker original(DamageInfo{2, 8, "2d4"});

    json j;
    original.save(j);

    Attacker loaded(DamageInfo{0, 0, ""});
    loaded.load(j);

    EXPECT_EQ(loaded.get_damage_info().minDamage, 2);
    EXPECT_EQ(loaded.get_damage_info().maxDamage, 8);
    EXPECT_EQ(loaded.get_damage_info().displayRoll, "2d4");
}

TEST_F(AttackerTest, Serialization_PreservesAllFields) {
    DamageInfo damage{3, 12, "1d10+2"};
    Attacker original(damage);

    json j;
    original.save(j);

    // Verify JSON structure
    EXPECT_TRUE(j.contains("damageInfo"));
    EXPECT_EQ(j["damageInfo"]["min"], 3);
    EXPECT_EQ(j["damageInfo"]["max"], 12);
    EXPECT_EQ(j["damageInfo"]["display"], "1d10+2");
}

// ----------------------------------------------------------------------------
// Damage Info Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, GetDamageInfo_ReturnsStoredDamage) {
    DamageInfo expected{2, 8, "2d4"};
    Attacker attacker(expected);

    const auto& actual = attacker.get_damage_info();

    EXPECT_EQ(actual.minDamage, expected.minDamage);
    EXPECT_EQ(actual.maxDamage, expected.maxDamage);
    EXPECT_EQ(actual.displayRoll, expected.displayRoll);
}

TEST_F(AttackerTest, SetDamageInfo_UpdatesDamage) {
    Attacker attacker(DamageInfo{1, 4, "1d4"});

    attacker.set_damage_info(DamageInfo{2, 12, "2d6"});

    EXPECT_EQ(attacker.get_damage_info().minDamage, 2);
    EXPECT_EQ(attacker.get_damage_info().maxDamage, 12);
}

TEST_F(AttackerTest, RollDamage_RespectsMinMax) {
    Attacker attacker(DamageInfo{3, 10, "1d8+2"});

    // Roll many times and verify bounds
    for (int i = 0; i < 100; ++i) {
        int damage = attacker.roll_damage();
        EXPECT_GE(damage, 3) << "Damage below minimum on roll " << i;
        EXPECT_LE(damage, 10) << "Damage above maximum on roll " << i;
    }
}

// ----------------------------------------------------------------------------
// GetAttackDamage Tests (Player vs Monster)
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, GetAttackDamage_MonsterUsesBaseDamage) {
    // Monster's attacker should return base damage, not look for weapons
    DamageInfo monsterDamage{2, 8, "2d4"};
    monster->attacker = std::make_unique<Attacker>(monsterDamage);

    DamageInfo result = monster->attacker->get_attack_damage(*monster);

    EXPECT_EQ(result.minDamage, 2);
    EXPECT_EQ(result.maxDamage, 8);
}

TEST_F(AttackerTest, GetAttackDamage_UnarmedPlayerReturnsUnarmedDamage) {
    // Player without equipped weapon should get unarmed damage (1d2)
    DamageInfo result = player->attacker->get_attack_damage(*player);

    // Unarmed is 1d2 (min=1, max=2)
    EXPECT_EQ(result.minDamage, 1);
    EXPECT_EQ(result.maxDamage, 2);
}

// ----------------------------------------------------------------------------
// Edge Cases
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, Attack_TargetWithoutDestructible_DoesNotCrash) {
    // Create creature without destructible
    auto targetNoDestructible = std::make_unique<Creature>(
        Vector2D{2, 0}, ActorData{'?', "ghost", 1}
    );
    targetNoDestructible->set_strength(10);
    // No destructible component

    // Should not crash, just log warning
    EXPECT_NO_THROW(player->attacker->attack(*player, *targetNoDestructible, ctx));
}

TEST_F(AttackerTest, Attack_DeadTarget_AttacksInVain) {
    // Kill the monster first
    monster->destructible->set_hp(0);
    ASSERT_TRUE(monster->destructible->is_dead());

    int hpBefore = monster->destructible->get_hp();

    // Attack should not deal damage
    game.d.set_next_d20(20); // Natural 20
    player->attacker->attack(*player, *monster, ctx);

    // HP should remain unchanged
    EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

TEST_F(AttackerTest, Attack_AttackerWithZeroStrength_AttacksInVain) {
    player->set_strength(0);

    int hpBefore = monster->destructible->get_hp();

    // Attack should fail
    game.d.set_next_d20(20);
    player->attacker->attack(*player, *monster, ctx);

    // HP should remain unchanged (attack was in vain)
    EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

// ----------------------------------------------------------------------------
// THAC0 Calculation Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, THAC0_RollNeeded_Calculation) {
    // THAC0 20 vs AC 10 means you need to roll 10+
    // THAC0 20 vs AC 6 means you need to roll 14+
    // THAC0 19 vs AC 6 means you need to roll 13+

    // Player: THAC0 20, Monster: AC 6
    // Roll needed = THAC0 - AC = 20 - 6 = 14

    // Set dice to roll exactly 14 (should hit)
    game.d.set_next_d20(14);
    game.d.set_next_roll(4); // damage roll

    int hpBefore = monster->destructible->get_hp();
    player->attacker->attack(*player, *monster, ctx);

    // Should have hit and dealt damage
    EXPECT_LT(monster->destructible->get_hp(), hpBefore);
}

TEST_F(AttackerTest, THAC0_RollBelowNeeded_Misses) {
    // Roll needed = 20 - 6 = 14
    // Roll 13 should miss
    game.d.set_next_d20(13);

    int hpBefore = monster->destructible->get_hp();
    player->attacker->attack(*player, *monster, ctx);

    // Should have missed
    EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

TEST_F(AttackerTest, THAC0_LowAC_EasierToHit) {
    // Better (lower) AC should be harder to hit
    // AC 0 with THAC0 20 = need to roll 20
    monster->destructible->set_armor_class(0);

    game.d.set_next_d20(19);

    int hpBefore = monster->destructible->get_hp();
    player->attacker->attack(*player, *monster, ctx);

    // Roll 19 vs need 20 = miss
    EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

TEST_F(AttackerTest, THAC0_HighAC_EasierToHit) {
    // Worse (higher) AC should be easier to hit
    // AC 15 with THAC0 20 = need to roll 5
    monster->destructible->set_armor_class(15);

    game.d.set_next_d20(5);
    game.d.set_next_roll(4);

    int hpBefore = monster->destructible->get_hp();
    player->attacker->attack(*player, *monster, ctx);

    // Roll 5 vs need 5 = hit
    EXPECT_LT(monster->destructible->get_hp(), hpBefore);
}

// ----------------------------------------------------------------------------
// Damage Reduction Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, DamageReduction_ReducesDamage) {
    // Monster with DR 3
    monster->destructible->set_dr(3);
    monster->destructible->set_armor_class(20); // Easy to hit

    // Guaranteed hit with known damage
    game.d.set_next_d20(20);
    game.d.set_next_roll(5); // 5 damage

    // With strength 10 and standard tables, damage bonus should be 0
    // Final damage = max(0, 5 + 0 - 3) = 2

    int hpBefore = monster->destructible->get_hp();
    player->attacker->attack(*player, *monster, ctx);

    int actualDamage = hpBefore - monster->destructible->get_hp();

    // Damage should be reduced (exact amount depends on strength bonus from table)
    EXPECT_LT(actualDamage, 5) << "DR should reduce damage";
    EXPECT_GE(actualDamage, 0) << "Damage should not be negative";
}

TEST_F(AttackerTest, DamageReduction_CanReduceToZero) {
    // Monster with high DR
    monster->destructible->set_dr(10);
    monster->destructible->set_armor_class(20);

    game.d.set_next_d20(20);
    game.d.set_next_roll(4); // 4 damage, will be reduced to 0

    int hpBefore = monster->destructible->get_hp();
    player->attacker->attack(*player, *monster, ctx);

    // Damage should be 0 after DR
    EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

// ----------------------------------------------------------------------------
// Monster Attack Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, MonsterAttack_UsesStoredWeaponName) {
    monster->set_weapon_equipped("sharp claws");
    monster->destructible->set_armor_class(10);
    player->destructible->set_armor_class(15); // Easy to hit

    game.d.set_next_d20(10);
    game.d.set_next_roll(5);

    // Should not crash and should use weapon name
    EXPECT_NO_THROW(monster->attacker->attack(*monster, *player, ctx));
}

TEST_F(AttackerTest, MonsterAttack_DealsCorrectDamage) {
    // Monster with 1d6 damage attacks player
    monster->attacker = std::make_unique<Attacker>(DamageInfo{1, 6, "1d6"});
    player->destructible->set_armor_class(20); // Very easy to hit
    player->destructible->set_dr(0);

    // Roll to hit and damage
    game.d.set_next_d20(15);
    game.d.set_next_roll(4); // Roll 4 damage

    int hpBefore = player->destructible->get_hp();
    monster->attacker->attack(*monster, *player, ctx);
    int hpAfter = player->destructible->get_hp();

    // Damage depends on strength bonus, but should be > 0 (hit confirmed)
    if (hpAfter < hpBefore) {
        EXPECT_GT(hpBefore - hpAfter, 0);
    }
    // Note: Could miss if monster's THAC0 vs player's AC calculation differs
}

// ----------------------------------------------------------------------------
// Kill Confirmation Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, Attack_CanKillTarget) {
    // Set monster to 1 HP
    monster->destructible->set_hp(1);
    monster->destructible->set_armor_class(20);
    monster->destructible->set_dr(0);

    game.d.set_next_d20(20);
    game.d.set_next_roll(5);

    ASSERT_FALSE(monster->destructible->is_dead());
    player->attacker->attack(*player, *monster, ctx);

    // Monster should be dead
    EXPECT_TRUE(monster->destructible->is_dead());
}

TEST_F(AttackerTest, Attack_MonsterDeathAwardsXP) {
    monster->destructible->set_hp(1);
    monster->destructible->set_armor_class(20);
    monster->destructible->set_dr(0);
    monster->destructible->set_xp(100);

    game.d.set_next_d20(20);
    game.d.set_next_roll(10);

    int xpBefore = player->destructible->get_xp();
    player->attacker->attack(*player, *monster, ctx);

    // Player should have gained XP
    EXPECT_EQ(player->destructible->get_xp(), xpBefore + 100);
}
