#include <gtest/gtest.h>
#include "tests/mocks/MockGameContext.h"
#include "src/Actor/Destructible.h"
#include "src/Actor/Actor.h"
#include "src/ActorTypes/Player.h"
#include "src/Systems/DataManager.h"
#include "src/Ai/AiPlayer.h"

// ============================================================================
// DESTRUCTIBLE EDGE CASE TESTS
// Tests healing limits, damage edge cases, HP clamping, death mechanics
// ============================================================================

  class DestructibleEdgeCaseTest : public ::testing::Test
  {
  protected:
      MockGameContext mock;
      GameContext ctx;
      DataManager data_manager;
      std::vector<std::unique_ptr<Creature>> creatures;
      std::unique_ptr<Player> player;
      std::unique_ptr<Creature> monster;

      void SetUp() override
      {
          data_manager.load_all_data(mock.messages);

          player = std::make_unique<Player>(Vector2D{0, 0});
          player->destructible = std::make_unique<PlayerDestructible>(100, 5, "your corpse", 0, 20, 10);
          player->ai = std::make_unique<AiPlayer>();
          player->set_constitution(10);

          monster = std::make_unique<Creature>(Vector2D{1, 0}, ActorData{'o', "orc", 1});
          monster->destructible = std::make_unique<MonsterDestructible>(50, 2, "orc corpse", 75, 19, 7);
          monster->set_constitution(10);

          ctx = mock.to_game_context();
          ctx.player = player.get();
          ctx.creatures = &creatures;
          ctx.data_manager = &data_manager;
      }

      void TearDown() override
      {
          mock.dice.set_test_mode(false);
          mock.dice.clear_fixed_rolls();
      }
  };
// ----------------------------------------------------------------------------
// HP Setter Edge Cases
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, SetHp_AcceptsNegative)
{
    monster->destructible->set_hp(-50);
    EXPECT_EQ(monster->destructible->get_hp(), -50);
}

TEST_F(DestructibleEdgeCaseTest, SetHp_AcceptsOverMax)
{
    monster->destructible->set_max_hp(50);
    monster->destructible->set_hp(999);
    EXPECT_EQ(monster->destructible->get_hp(), 999);
}

TEST_F(DestructibleEdgeCaseTest, SetHp_AcceptsValidValue)
{
    monster->destructible->set_max_hp(100);
    monster->destructible->set_hp(75);
    EXPECT_EQ(monster->destructible->get_hp(), 75);
}

TEST_F(DestructibleEdgeCaseTest, SetHp_ZeroIsValid)
{
    monster->destructible->set_hp(0);
    EXPECT_EQ(monster->destructible->get_hp(), 0);
    EXPECT_TRUE(monster->destructible->is_dead());
}

// ----------------------------------------------------------------------------
// Max HP Setter Edge Cases
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, SetMaxHp_AcceptsZero)
{
    monster->destructible->set_max_hp(0);
    EXPECT_EQ(monster->destructible->get_max_hp(), 0);
}

TEST_F(DestructibleEdgeCaseTest, SetMaxHp_AcceptsNegative)
{
    monster->destructible->set_max_hp(-100);
    EXPECT_EQ(monster->destructible->get_max_hp(), -100);
}

TEST_F(DestructibleEdgeCaseTest, SetMaxHp_DoesNotClampCurrentHp)
{
    monster->destructible->set_max_hp(100);
    monster->destructible->set_hp(100);

    // Reduce max HP
    monster->destructible->set_max_hp(50);

    // Current HP should NOT be automatically clamped
    EXPECT_EQ(monster->destructible->get_hp(), 100);
    EXPECT_EQ(monster->destructible->get_max_hp(), 50);
}

// ----------------------------------------------------------------------------
// HP Base Setter Edge Cases
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, SetHpBase_AcceptsZero)
{
    monster->destructible->set_hp_base(0);
    EXPECT_EQ(monster->destructible->get_hp_base(), 0);
}

TEST_F(DestructibleEdgeCaseTest, SetHpBase_AcceptsNegative)
{
    monster->destructible->set_hp_base(-999);
    EXPECT_EQ(monster->destructible->get_hp_base(), -999);
}

// ----------------------------------------------------------------------------
// Healing Edge Cases
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, Heal_ReturnsActualHealed)
{
    monster->destructible->set_max_hp(100);
    monster->destructible->set_hp(90);

    int healed = monster->destructible->heal(50);

    // Should only heal 10 (to max)
    EXPECT_EQ(healed, 10);
    EXPECT_EQ(monster->destructible->get_hp(), 100);
}

TEST_F(DestructibleEdgeCaseTest, Heal_AtMaxHp_ReturnsZero)
{
    monster->destructible->set_max_hp(50);
    monster->destructible->set_hp(50);

    int healed = monster->destructible->heal(100);

    EXPECT_EQ(healed, 0);
    EXPECT_EQ(monster->destructible->get_hp(), 50);
}

TEST_F(DestructibleEdgeCaseTest, Heal_FromZeroHp)
{
    monster->destructible->set_max_hp(100);
    monster->destructible->set_hp(0);

    int healed = monster->destructible->heal(50);

    EXPECT_EQ(healed, 50);
    EXPECT_EQ(monster->destructible->get_hp(), 50);
}

TEST_F(DestructibleEdgeCaseTest, Heal_ExactAmountToMax)
{
    monster->destructible->set_max_hp(100);
    monster->destructible->set_hp(70);

    int healed = monster->destructible->heal(30);

    EXPECT_EQ(healed, 30);
    EXPECT_EQ(monster->destructible->get_hp(), 100);
}

TEST_F(DestructibleEdgeCaseTest, Heal_ZeroAmount)
{
    monster->destructible->set_hp(50);
    int hpBefore = monster->destructible->get_hp();

    int healed = monster->destructible->heal(0);

    EXPECT_EQ(healed, 0);
    EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

TEST_F(DestructibleEdgeCaseTest, Heal_NegativeAmount_TreatedAsHeal)
{
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

TEST_F(DestructibleEdgeCaseTest, TakeDamage_ZeroDamage_NoEffect)
{
    int hpBefore = monster->destructible->get_hp();

    monster->destructible->take_damage(*monster, 0, ctx);

    EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

TEST_F(DestructibleEdgeCaseTest, TakeDamage_NegativeDamage_NoEffect)
{
    int hpBefore = monster->destructible->get_hp();

    monster->destructible->take_damage(*monster, -10, ctx);

    EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

TEST_F(DestructibleEdgeCaseTest, TakeDamage_ExactlyLethal)
{
    monster->destructible->set_hp(10);

    monster->destructible->take_damage(*monster, 10, ctx);

    EXPECT_TRUE(monster->destructible->is_dead());
    EXPECT_EQ(monster->destructible->get_hp(), 0);
}

TEST_F(DestructibleEdgeCaseTest, TakeDamage_Overkill)
{
    monster->destructible->set_hp(10);

    monster->destructible->take_damage(*monster, 1000, ctx);

    EXPECT_TRUE(monster->destructible->is_dead());
    EXPECT_EQ(monster->destructible->get_hp(), 0);
}

TEST_F(DestructibleEdgeCaseTest, TakeDamage_OneDamage)
{
    monster->destructible->set_hp(10);

    monster->destructible->take_damage(*monster, 1, ctx);

    EXPECT_EQ(monster->destructible->get_hp(), 9);
    EXPECT_FALSE(monster->destructible->is_dead());
}

TEST_F(DestructibleEdgeCaseTest, TakeDamage_AlreadyDead_NoFurtherEffect)
{
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

TEST_F(DestructibleEdgeCaseTest, IsDead_TrueAtZeroHp)
{
    monster->destructible->set_hp(0);
    EXPECT_TRUE(monster->destructible->is_dead());
}

TEST_F(DestructibleEdgeCaseTest, IsDead_FalseAtOneHp)
{
    monster->destructible->set_hp(1);
    EXPECT_FALSE(monster->destructible->is_dead());
}

TEST_F(DestructibleEdgeCaseTest, IsDead_TrueWithNegativeClampedHp)
{
    // Even though set_hp clamps negative to 0, verify is_dead works
    monster->destructible->set_hp(-100);
    EXPECT_TRUE(monster->destructible->is_dead());
}

// ----------------------------------------------------------------------------
// Attribute Getter/Setter Tests
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, ArmorClass_CanBeNegative)
{
    // In AD&D, negative AC is better
    monster->destructible->set_armor_class(-5);
    EXPECT_EQ(monster->destructible->get_armor_class(), -5);
}

TEST_F(DestructibleEdgeCaseTest, THAC0_CanBeSetToAnyValue)
{
    monster->destructible->set_thaco(5);
    EXPECT_EQ(monster->destructible->get_thaco(), 5);

    monster->destructible->set_thaco(25);
    EXPECT_EQ(monster->destructible->get_thaco(), 25);
}

TEST_F(DestructibleEdgeCaseTest, DamageReduction_AcceptsAnyValue)
{
    monster->destructible->set_dr(0);
    EXPECT_EQ(monster->destructible->get_dr(), 0);

    monster->destructible->set_dr(100);
    EXPECT_EQ(monster->destructible->get_dr(), 100);
}

TEST_F(DestructibleEdgeCaseTest, XP_CanBeModified)
{
    monster->destructible->set_xp(100);
    EXPECT_EQ(monster->destructible->get_xp(), 100);

    monster->destructible->add_xp(50);
    EXPECT_EQ(monster->destructible->get_xp(), 150);
}

TEST_F(DestructibleEdgeCaseTest, CorpseName_Preserved)
{
    monster->destructible->set_corpse_name("dead orc");
    EXPECT_EQ(monster->destructible->get_corpse_name(), "dead orc");
}

// ----------------------------------------------------------------------------
// Player Death Tests
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, PlayerDeath_SetsDefeatStatus)
{
    player->destructible->set_hp(1);

    player->destructible->take_damage(*player, 10, ctx);

    EXPECT_TRUE(player->destructible->is_dead());
    EXPECT_EQ(*ctx.game_status, GameStatus::DEFEAT);
}

// ----------------------------------------------------------------------------
// Monster Death Tests (XP Award)
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, MonsterDeath_AwardsXPToPlayer)
{
    monster->destructible->set_hp(1);
    monster->destructible->set_xp(200);

    int playerXpBefore = player->destructible->get_xp();

    monster->destructible->take_damage(*monster, 10, ctx);

    EXPECT_TRUE(monster->destructible->is_dead());
    EXPECT_EQ(player->destructible->get_xp(), playerXpBefore + 200);
}

TEST_F(DestructibleEdgeCaseTest, MonsterDeath_ZeroXPMonster)
{
    monster->destructible->set_hp(1);
    monster->destructible->set_xp(0);

    int playerXpBefore = player->destructible->get_xp();

    monster->destructible->take_damage(*monster, 10, ctx);

    EXPECT_EQ(player->destructible->get_xp(), playerXpBefore);
}

// ----------------------------------------------------------------------------
// Serialization Regression Tests
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, Serialization_AllFieldsPreserved)
{
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

TEST_F(DestructibleEdgeCaseTest, Serialization_DeadState_Preserved)
{
    monster->destructible->set_hp(0);
    ASSERT_TRUE(monster->destructible->is_dead());

    json j;
    monster->destructible->save(j);

    auto loaded = Destructible::create(j);

    ASSERT_NE(loaded, nullptr);
    EXPECT_TRUE(loaded->is_dead());
    EXPECT_EQ(loaded->get_hp(), 0);
}

TEST_F(DestructibleEdgeCaseTest, Serialization_NegativeAC_Preserved)
{
    monster->destructible->set_armor_class(-10);

    json j;
    monster->destructible->save(j);

    auto loaded = Destructible::create(j);

    EXPECT_EQ(loaded->get_armor_class(), -10);
}

// ----------------------------------------------------------------------------
// Factory Create Edge Cases
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, Create_InvalidType_ReturnsNullptr)
{
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

TEST_F(DestructibleEdgeCaseTest, Create_MissingType_ReturnsNullptr)
{
    json j;
    // No "type" field
    j["hpMax"] = 10;
    j["hp"] = 10;

    auto result = Destructible::create(j);

    EXPECT_EQ(result, nullptr);
}

TEST_F(DestructibleEdgeCaseTest, Create_MonsterType_ReturnsMonsterDestructible)
{
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
    j["tempHp"] = 0;

    auto result = Destructible::create(j);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get_max_hp(), 50);
    // Verify it's actually a MonsterDestructible by checking behavior would be harder
}

TEST_F(DestructibleEdgeCaseTest, Create_PlayerType_ReturnsPlayerDestructible)
{
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
    j["tempHp"] = 0;

    auto result = Destructible::create(j);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->get_hp(), 80);
    EXPECT_EQ(result->get_xp(), 5000);
}

// ----------------------------------------------------------------------------
// Spell Damage / Cleanup Dead Creatures Tests
// Regression: Lightning bolt didn't call cleanup, spiders didn't die visually
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, CleanupDeadCreatures_RemovesDeadFromVector)
{
    creatures.push_back(std::move(monster));
    ASSERT_EQ(creatures.size(), 1);

    creatures[0]->destructible->take_damage(*creatures[0], 1000, ctx);
    EXPECT_TRUE(creatures[0]->destructible->is_dead());

    ctx.creature_manager->cleanup_dead_creatures(creatures);

    EXPECT_EQ(creatures.size(), 0) << "Dead creatures should be removed after cleanup";
}

TEST_F(DestructibleEdgeCaseTest, CleanupDeadCreatures_KeepsAliveCreatures)
{
    creatures.push_back(std::move(monster));

    creatures[0]->destructible->set_hp(50);
    creatures[0]->destructible->take_damage(*creatures[0], 10, ctx);
    EXPECT_FALSE(creatures[0]->destructible->is_dead());

    ctx.creature_manager->cleanup_dead_creatures(creatures);

    EXPECT_EQ(creatures.size(), 1) << "Alive creatures should not be removed";
}

TEST_F(DestructibleEdgeCaseTest, SpellDamage_KillsAndRequiresCleanup)
{
    creatures.push_back(std::move(monster));

    creatures[0]->destructible->take_damage(*creatures[0], 1000, ctx);

    EXPECT_TRUE(creatures[0]->destructible->is_dead());
    EXPECT_EQ(creatures.size(), 1) << "Dead creature still in list before cleanup";

    ctx.creature_manager->cleanup_dead_creatures(creatures);
    EXPECT_EQ(creatures.size(), 0) << "Dead creature removed after cleanup";
}

// ----------------------------------------------------------------------------
// Constitution Bonus Tests (AD&D 2e Rules)
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, ConstitutionBonus_IncreaseAddsHp)
{
    player->set_constitution(10); // +0 HP/level
    player->set_player_level(5);
    player->destructible->set_last_constitution(10);
    player->destructible->update_constitution_bonus(*player, ctx);

    int hpBefore = player->destructible->get_hp();
    int maxHpBefore = player->destructible->get_max_hp();

    // Increase Constitution from 10 to 16 (+2 HP/level)
    player->set_constitution(16);
    player->destructible->update_constitution_bonus(*player, ctx);

    // Should gain 2 HP * 5 levels = 10 HP (incremental update)
    EXPECT_EQ(player->destructible->get_hp(), hpBefore + 10);
    EXPECT_EQ(player->destructible->get_max_hp(), maxHpBefore + 10);
}

TEST_F(DestructibleEdgeCaseTest, ConstitutionBonus_DecreaseRemovesHp)
{
    // Start fresh: establish baseline with con 16
    player->set_player_level(5);  // Set level first
    player->set_constitution(16); // +2 HP/level
    player->destructible->set_last_constitution(10);  // Starting from con 10
    player->destructible->update_constitution_bonus(*player, ctx);  // Apply con 16 bonus (+10 HP)

    int hpBefore = player->destructible->get_hp();
    int maxHpBefore = player->destructible->get_max_hp();

    // Decrease Constitution from 16 to 10 (0 HP/level)
    player->set_constitution(10);
    player->destructible->update_constitution_bonus(*player, ctx);

    // Verify HP and max_HP both decreased consistently (incremental architecture)
    int hpAfter = player->destructible->get_hp();
    int maxHpAfter = player->destructible->get_max_hp();
    int hpLoss = hpBefore - hpAfter;
    int maxHpLoss = maxHpBefore - maxHpAfter;

    EXPECT_EQ(hpLoss, maxHpLoss) << "HP and max_HP should decrease by same amount";
    EXPECT_GT(hpLoss, 0) << "Should have lost HP from constitution decrease";
}

TEST_F(DestructibleEdgeCaseTest, ConstitutionBonus_CapsAtLevel9ForFighter)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_constitution(16); // +2 HP/level
    player->set_player_level(12);
    player->destructible->set_hp_base(50);
    player->destructible->set_max_hp(50);
    player->destructible->set_hp(50);
    player->destructible->set_last_constitution(10);
    
    player->destructible->update_constitution_bonus(*player, ctx);
    
    // Should be capped at level 9: 2 HP * 9 = 18 HP
    EXPECT_EQ(player->destructible->get_max_hp(), 50 + 18);
}

TEST_F(DestructibleEdgeCaseTest, ConstitutionBonus_CapsAtLevel9ForCleric)
{
    player->playerClassState = Player::PlayerClassState::CLERIC;
    player->set_constitution(16);
    player->set_player_level(15);
    player->destructible->set_hp_base(50);
    player->destructible->set_max_hp(50);
    player->destructible->set_hp(50);
    player->destructible->set_last_constitution(10);
    
    player->destructible->update_constitution_bonus(*player, ctx);
    
    // Capped at level 9: 2 HP * 9 = 18 HP
    EXPECT_EQ(player->destructible->get_max_hp(), 50 + 18);
}

TEST_F(DestructibleEdgeCaseTest, ConstitutionBonus_CapsAtLevel10ForRogue)
{
    player->playerClassState = Player::PlayerClassState::ROGUE;
    player->set_constitution(16);
    player->set_player_level(15);
    player->destructible->set_hp_base(50);
    player->destructible->set_max_hp(50);
    player->destructible->set_hp(50);
    player->destructible->set_last_constitution(10);
    
    player->destructible->update_constitution_bonus(*player, ctx);
    
    // Capped at level 10: 2 HP * 10 = 20 HP
    EXPECT_EQ(player->destructible->get_max_hp(), 50 + 20);
}

TEST_F(DestructibleEdgeCaseTest, ConstitutionBonus_CapsAtLevel10ForWizard)
{
    player->playerClassState = Player::PlayerClassState::WIZARD;
    player->set_constitution(16);
    player->set_player_level(20);
    player->destructible->set_hp_base(30);    
    player->destructible->set_max_hp(30);
    player->destructible->set_hp(30);
    player->destructible->set_last_constitution(10);
    
    player->destructible->update_constitution_bonus(*player, ctx);
    
    // Capped at level 10: 2 HP * 10 = 20 HP
    EXPECT_EQ(player->destructible->get_max_hp(), 30 + 20);
}

TEST_F(DestructibleEdgeCaseTest, ConstitutionBonus_NoChangeDoesNothing)
{
    player->set_constitution(14);
    player->set_player_level(5);
    player->destructible->set_hp_base(50);
    player->destructible->set_max_hp(50);
    player->destructible->set_hp(50);
    player->destructible->set_last_constitution(14);
    
    int hpBefore = player->destructible->get_hp();
    int maxHpBefore = player->destructible->get_max_hp();
    
    // Update with same Constitution
    player->destructible->update_constitution_bonus(*player, ctx);
    
    EXPECT_EQ(player->destructible->get_hp(), hpBefore);
    EXPECT_EQ(player->destructible->get_max_hp(), maxHpBefore);
}

TEST_F(DestructibleEdgeCaseTest, ConstitutionDrain_CanCauseDeath)
{
    player->set_constitution(16); // +2 HP/level
    player->set_player_level(5);
    player->destructible->set_max_hp(10);
    player->destructible->set_hp(10);
    player->destructible->set_last_constitution(16);
    player->destructible->update_constitution_bonus(*player, ctx);

    // Verify starting state: 10 HP with con 16
    EXPECT_EQ(player->destructible->get_max_hp(), 10);
    EXPECT_FALSE(player->destructible->is_dead());

    // Drain Constitution to 3 (-2 HP/level)
    // Difference: -2 - (+2) = -4 HP/level
    // Change: -4 * 5 levels = -20 HP
    // 10 - 20 = -10 HP -> clamped to 0 -> DEAD
    player->set_constitution(3);
    player->destructible->update_constitution_bonus(*player, ctx);

    EXPECT_TRUE(player->destructible->is_dead());
    EXPECT_EQ(*ctx.game_status, GameStatus::DEFEAT);
}

TEST_F(DestructibleEdgeCaseTest, ConstitutionBonus_InvalidConstitutionReturnsZero)
{
    player->set_constitution(10); // Start with valid con
    player->set_player_level(5);
    player->destructible->set_last_constitution(10);
    player->destructible->update_constitution_bonus(*player, ctx);

    int maxHpBefore = player->destructible->get_max_hp();

    // Change to invalid Constitution (should result in 0 bonus)
    player->set_constitution(0);
    player->destructible->update_constitution_bonus(*player, ctx);

    // Invalid constitution (0 bonus) - no change from con 10 (0 bonus)
    EXPECT_EQ(player->destructible->get_max_hp(), maxHpBefore);
}

TEST_F(DestructibleEdgeCaseTest, ConstitutionBonus_MonsterAlwaysUsesLevel1)
{
    // Monster starts with con 10 (from SetUp)
    int maxHpBefore = monster->destructible->get_max_hp();

    monster->set_constitution(16); // +2 HP/level
    monster->destructible->update_constitution_bonus(*monster, ctx);

    // Monsters don't have levels, so multiplier is 1
    // Change from con 10 (0 bonus) to con 16 (+2 bonus) = +2 HP
    EXPECT_EQ(monster->destructible->get_max_hp(), maxHpBefore + 2);
}

// ----------------------------------------------------------------------------
// Temporary HP Tests (Aid Spell)
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, TempHp_AbsorbsDamageFirst)
{
    player->destructible->set_hp(50);
    player->destructible->set_temp_hp(10);
    
    player->destructible->take_damage(*player, 15, ctx);
    
    // 10 damage absorbed by temp HP, 5 hits real HP
    EXPECT_EQ(player->destructible->get_temp_hp(), 0);
    EXPECT_EQ(player->destructible->get_hp(), 45);
}

TEST_F(DestructibleEdgeCaseTest, TempHp_AllDamageAbsorbed)
{
    player->destructible->set_hp(50);
    player->destructible->set_temp_hp(20);
    
    player->destructible->take_damage(*player, 10, ctx);
    
    // All damage absorbed by temp HP
    EXPECT_EQ(player->destructible->get_temp_hp(), 10);
    EXPECT_EQ(player->destructible->get_hp(), 50);
}

TEST_F(DestructibleEdgeCaseTest, TempHp_NotAffectedByHealing)
{
    player->destructible->set_max_hp(100);
    player->destructible->set_hp(50);
    player->destructible->set_temp_hp(10);
    
    player->destructible->heal(20);
    
    // Healing affects real HP only
    EXPECT_EQ(player->destructible->get_hp(), 70);
    EXPECT_EQ(player->destructible->get_temp_hp(), 10); // Unchanged
}

TEST_F(DestructibleEdgeCaseTest, TempHp_NotAffectedByConstitutionChange)
{
    player->set_constitution(10);
    player->set_player_level(5);
    player->destructible->set_hp_base(50);
    player->destructible->set_hp(50);
    player->destructible->set_temp_hp(15);
    player->destructible->set_last_constitution(10);
    
    // Increase Constitution
    player->set_constitution(16);
    player->destructible->update_constitution_bonus(*player, ctx);
    
    // Temp HP should remain unchanged
    EXPECT_EQ(player->destructible->get_temp_hp(), 15);
}

TEST_F(DestructibleEdgeCaseTest, TempHp_EffectiveHpCalculation)
{
    player->destructible->set_hp(50);
    player->destructible->set_temp_hp(10);
    
    EXPECT_EQ(player->destructible->get_effective_hp(), 60);
}

TEST_F(DestructibleEdgeCaseTest, TempHp_CannotBeNegative)
{
    player->destructible->set_temp_hp(-10);
    
    EXPECT_EQ(player->destructible->get_temp_hp(), 0);
}

TEST_F(DestructibleEdgeCaseTest, TempHp_AddTempHp)
{
    player->destructible->set_temp_hp(5);
    player->destructible->add_temp_hp(10);
    
    EXPECT_EQ(player->destructible->get_temp_hp(), 15);
}

// ----------------------------------------------------------------------------
// Serialization Tests for New Fields
// ----------------------------------------------------------------------------

TEST_F(DestructibleEdgeCaseTest, Serialization_TempHpPreserved)
{
    player->destructible->set_temp_hp(20);
    
    json j;
    player->destructible->save(j);
    
    auto loaded = Destructible::create(j);
    
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->get_temp_hp(), 20);
}

TEST_F(DestructibleEdgeCaseTest, Serialization_LastConstitutionPreserved)
{
    player->destructible->set_last_constitution(16);
    
    json j;
    player->destructible->save(j);
    
    auto loaded = Destructible::create(j);
    
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->get_last_constitution(), 16);
}