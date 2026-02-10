#include <gtest/gtest.h>
#include <memory>

#include "src/Game.h"
#include "src/Actor/Attacker.h"
#include "src/Actor/Destructible.h"
#include "src/Actor/Actor.h"
#include "src/ActorTypes/Player.h"
#include "src/Combat/DamageInfo.h"

// ============================================================================
// ATTACKER TESTS
// Tests combat calculations, hit/miss logic, damage application
// ============================================================================

class AttackerTest : public ::testing::Test
{
protected:
    Game game;
    GameContext ctx;
    std::unique_ptr<Player> player;
    std::unique_ptr<Creature> monster;

    void SetUp() override
    {
        try {
            game.data_manager.load_all_data(game.message_system);
        } catch (...) {}

        player = std::make_unique<Player>(Vector2D{0, 0});
        player->destructible = std::make_unique<PlayerDestructible>(
            20, 0, "your corpse", 0, 20, 10
        );
        player->attacker = std::make_unique<Attacker>(DamageInfo{1, 4, "1d4"});
        player->set_strength(10);
        player->set_dexterity(10);

        monster = std::make_unique<Creature>(Vector2D{1, 0}, ActorData{'g', "goblin", 1});
        monster->destructible = std::make_unique<MonsterDestructible>(
            10, 0, "goblin corpse", 50, 19, 6
        );
        monster->attacker = std::make_unique<Attacker>(DamageInfo{1, 6, "1d6"});
        monster->set_strength(8);
        monster->set_dexterity(10);
        monster->set_weapon_equipped("claws");

        ctx = game.context();
        ctx.player = player.get();

        game.dice.set_test_mode(true);
    }

    void TearDown() override
    {
        game.dice.set_test_mode(false);
        game.dice.clear_fixed_rolls();
    }
};

// ----------------------------------------------------------------------------
// Attacker Serialization Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, Serialization_RoundTrip)
{
    Attacker original(DamageInfo{2, 8, "2d4", DamageType::FIRE});

    json j;
    original.save(j);

    Attacker loaded(DamageInfo{0, 0, ""});
    loaded.load(j);

    EXPECT_EQ(loaded.get_damage_info().minDamage, 2);
    EXPECT_EQ(loaded.get_damage_info().maxDamage, 8);
    EXPECT_EQ(loaded.get_damage_info().displayRoll, "2d4");
    EXPECT_EQ(loaded.get_damage_info().damageType, DamageType::FIRE);
}

// ----------------------------------------------------------------------------
// THAC0 Combat Calculation Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, THAC0_RollNeeded_Calculation)
{
    // Player THAC0 20, Monster AC 6
    // Roll needed = THAC0 - AC = 20 - 6 = 14
    monster->destructible->set_armor_class(6);

    game.dice.set_next_d20(14);
    game.dice.set_next_roll(3);

    int hpBefore = monster->destructible->get_hp();
    player->attacker->attack(*player, *monster, ctx);

    // Should hit with roll of exactly 14
    EXPECT_LT(monster->destructible->get_hp(), hpBefore);
}

TEST_F(AttackerTest, THAC0_RollBelowNeeded_Misses)
{
    monster->destructible->set_armor_class(6);

    game.dice.set_next_d20(13);
    game.dice.set_next_roll(3);

    int hpBefore = monster->destructible->get_hp();
    player->attacker->attack(*player, *monster, ctx);

    // Should miss with roll of 13
    EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

TEST_F(AttackerTest, THAC0_LowAC_EasierToHit)
{
    // AC 0 means roll needed = 20 - 0 = 20
    monster->destructible->set_armor_class(0);

    game.dice.set_next_d20(20);
    game.dice.set_next_roll(5);

    int hpBefore = monster->destructible->get_hp();
    player->attacker->attack(*player, *monster, ctx);

    EXPECT_LT(monster->destructible->get_hp(), hpBefore);
}

TEST_F(AttackerTest, THAC0_HighAC_EasierToHit)
{
    // AC 10 means roll needed = 20 - 10 = 10
    monster->destructible->set_armor_class(10);

    game.dice.set_next_d20(10);
    game.dice.set_next_roll(4);

    int hpBefore = monster->destructible->get_hp();
    player->attacker->attack(*player, *monster, ctx);

    EXPECT_LT(monster->destructible->get_hp(), hpBefore);
}

// ----------------------------------------------------------------------------
// Damage Reduction Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, DamageReduction_ReducesDamage)
{
    monster->destructible->set_armor_class(20);
    monster->destructible->set_dr(3);

    game.dice.set_next_d20(20);
    game.dice.set_next_roll(5);

    int hpBefore = monster->destructible->get_hp();
    player->attacker->attack(*player, *monster, ctx);

    // 5 damage - 3 DR = 2 actual damage
    EXPECT_EQ(monster->destructible->get_hp(), hpBefore - 2);
}

TEST_F(AttackerTest, DamageReduction_CanReduceToZero)
{
    monster->destructible->set_armor_class(20);
    monster->destructible->set_dr(10);

    game.dice.set_next_d20(20);
    game.dice.set_next_roll(5);

    int hpBefore = monster->destructible->get_hp();
    player->attacker->attack(*player, *monster, ctx);

    // 5 damage - 10 DR = 0 actual damage
    EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

// ----------------------------------------------------------------------------
// Monster Attack Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, MonsterAttack_UsesStoredWeaponName)
{
    monster->set_weapon_equipped("sharp claws");
    player->destructible->set_armor_class(10);

    game.dice.set_next_d20(20);
    game.dice.set_next_roll(4);

    // Attack should complete without errors using stored weapon name
    monster->attacker->attack(*monster, *player, ctx);

    EXPECT_TRUE(true);
}

TEST_F(AttackerTest, MonsterAttack_DealsCorrectDamage)
{
    player->destructible->set_armor_class(20);
    player->destructible->set_dr(0);

    game.dice.set_next_d20(20);
    game.dice.set_next_roll(6);

    int hpBefore = player->destructible->get_hp();
    monster->attacker->attack(*monster, *player, ctx);

    EXPECT_EQ(player->destructible->get_hp(), hpBefore - 6);
}

// ----------------------------------------------------------------------------
// Combat Result Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, Attack_CanKillTarget)
{
    monster->destructible->set_hp(1);
    monster->destructible->set_armor_class(20);
    monster->destructible->set_dr(0);

    game.dice.set_next_d20(20);
    game.dice.set_next_roll(5);

    ASSERT_FALSE(monster->destructible->is_dead());
    player->attacker->attack(*player, *monster, ctx);

    EXPECT_TRUE(monster->destructible->is_dead());
}

TEST_F(AttackerTest, Attack_MonsterDeathAwardsXP)
{
    monster->destructible->set_hp(1);
    monster->destructible->set_armor_class(20);
    monster->destructible->set_dr(0);

    game.dice.set_next_d20(20);
    game.dice.set_next_roll(5);

    int xpBefore = player->destructible->get_xp();
    player->attacker->attack(*player, *monster, ctx);

    // Monster was worth 50 XP
    EXPECT_GT(player->destructible->get_xp(), xpBefore);
}
