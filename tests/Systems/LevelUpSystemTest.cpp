#include <gtest/gtest.h>
#include "src/Game.h"
#include "src/Systems/LevelUpSystem.h"
#include "src/ActorTypes/Player.h"
#include "src/Actor/Destructible.h"

class LevelUpSystemTest : public ::testing::Test
{
protected:
    Game game;
    GameContext ctx;
    std::unique_ptr<Player> player;

    void SetUp() override
    {
        try {
            game.data_manager.load_all_data(game.message_system);
        } catch (...) {}

        player = std::make_unique<Player>(Vector2D{0, 0});
        player->destructible = std::make_unique<PlayerDestructible>(
            10, 0, "your corpse", 0, 0, 10
        );
        player->destructible->set_hp_base(10);

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

TEST_F(LevelUpSystemTest, CalculateBackstabMultiplier)
{
    EXPECT_EQ(LevelUpSystem::calculate_backstab_multiplier(1), 2);
    EXPECT_EQ(LevelUpSystem::calculate_backstab_multiplier(4), 2);
    EXPECT_EQ(LevelUpSystem::calculate_backstab_multiplier(5), 3);
    EXPECT_EQ(LevelUpSystem::calculate_backstab_multiplier(8), 3);
    EXPECT_EQ(LevelUpSystem::calculate_backstab_multiplier(9), 4);
    EXPECT_EQ(LevelUpSystem::calculate_backstab_multiplier(12), 4);
    EXPECT_EQ(LevelUpSystem::calculate_backstab_multiplier(13), 5);
    EXPECT_EQ(LevelUpSystem::calculate_backstab_multiplier(20), 5);
}

TEST_F(LevelUpSystemTest, FighterLevelUpHPGain)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_creature_class(CreatureClass::FIGHTER);
    player->set_hit_die(10);
    player->set_constitution(10);

    player->destructible->set_hp_base(10);
    player->destructible->set_max_hp(10);
    player->destructible->set_hp(10);

    game.dice.set_next_d20(8);

    LevelUpSystem::apply_level_up_benefits(*player, 2, &ctx);

    EXPECT_EQ(player->destructible->get_hp_base(), 18);
    EXPECT_EQ(player->destructible->get_max_hp(), 18);
    EXPECT_EQ(player->destructible->get_hp(), 18);
}

TEST_F(LevelUpSystemTest, WizardLevelUpHPGainWithConBonus)
{
    player->playerClassState = Player::PlayerClassState::WIZARD;
    player->set_creature_class(CreatureClass::WIZARD);
    player->set_hit_die(4);
    player->set_constitution(16);

    int expectedBonus = 0;
    if (!game.data_manager.get_constitution_attributes().empty()) {
        if (player->get_constitution() <= static_cast<int>(game.data_manager.get_constitution_attributes().size())) {
            expectedBonus = game.data_manager.get_constitution_attributes()[player->get_constitution() - 1].HPAdj;
        }
    }

    player->destructible->set_hp_base(4);
    player->destructible->set_max_hp(4 + expectedBonus);
    player->destructible->set_hp(4 + expectedBonus);

    int oldMaxHP = player->destructible->get_max_hp();

    game.dice.set_next_d20(3);

    LevelUpSystem::apply_level_up_benefits(*player, 2, &ctx);

    int gained = 3 + expectedBonus;
    EXPECT_EQ(player->destructible->get_max_hp(), oldMaxHP + gained);
}

TEST_F(LevelUpSystemTest, FighterExtraAttackAtLevel7)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_creature_class(CreatureClass::FIGHTER);
    player->set_hit_die(10);
    player->set_attacks_per_round(1.0f);

    LevelUpSystem::apply_level_up_benefits(*player, 7, &ctx);

    EXPECT_FLOAT_EQ(player->get_attacks_per_round(), 1.5f);
}

TEST_F(LevelUpSystemTest, FighterExtraAttackAtLevel13)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_creature_class(CreatureClass::FIGHTER);
    player->set_hit_die(10);
    player->set_attacks_per_round(1.5f);

    LevelUpSystem::apply_level_up_benefits(*player, 13, &ctx);

    EXPECT_FLOAT_EQ(player->get_attacks_per_round(), 2.0f);
}

TEST_F(LevelUpSystemTest, AbilityScoreImprovement)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_creature_class(CreatureClass::FIGHTER);
    player->set_hit_die(10);
    player->set_strength(15);

    LevelUpSystem::apply_level_up_benefits(*player, 4, &ctx);

    EXPECT_EQ(player->get_strength(), 16);
}

TEST_F(LevelUpSystemTest, NoAbilityScoreImprovementAtInterimLevel)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_creature_class(CreatureClass::FIGHTER);
    player->set_hit_die(10);
    player->set_strength(15);

    LevelUpSystem::apply_level_up_benefits(*player, 5, &ctx);

    EXPECT_EQ(player->get_strength(), 15);
}

TEST_F(LevelUpSystemTest, FighterTHAC0Improvement)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_creature_class(CreatureClass::FIGHTER);
    player->set_hit_die(10);
    player->destructible->set_thaco(20);

    game.dice.set_next_d20(5);

    LevelUpSystem::apply_level_up_benefits(*player, 2, &ctx);

    EXPECT_EQ(player->destructible->get_thaco(), 19);
}

TEST_F(LevelUpSystemTest, FighterExtraAttackSkippedLevel)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_creature_class(CreatureClass::FIGHTER);
    player->set_hit_die(10);
    player->set_attacks_per_round(1.0f);

    game.dice.set_next_d20(5);

    LevelUpSystem::apply_level_up_benefits(*player, 8, &ctx);

    EXPECT_FLOAT_EQ(player->get_attacks_per_round(), 1.5f);
}

// AD&D 2e XP table compliance — get_next_level_xp now lives on Player
TEST_F(LevelUpSystemTest, XpTableFighterLevel1)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_creature_level(1);
    EXPECT_EQ(player->get_next_level_xp(ctx), 2000);
}

TEST_F(LevelUpSystemTest, XpTableFighterLevel5)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_creature_level(5);
    EXPECT_EQ(player->get_next_level_xp(ctx), 32000);
}

TEST_F(LevelUpSystemTest, XpTableFighterLinearExtrapolation)
{
    // Level 11 = last table entry (750000) + 1 * 250000 = 1,000,000
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_creature_level(11);
    EXPECT_EQ(player->get_next_level_xp(ctx), 1000000);
}

TEST_F(LevelUpSystemTest, XpTableRogueLevel1)
{
    player->playerClassState = Player::PlayerClassState::ROGUE;
    player->set_creature_level(1);
    EXPECT_EQ(player->get_next_level_xp(ctx), 1250);
}

TEST_F(LevelUpSystemTest, XpTableClericLevel1)
{
    player->playerClassState = Player::PlayerClassState::CLERIC;
    player->set_creature_level(1);
    EXPECT_EQ(player->get_next_level_xp(ctx), 1500);
}

TEST_F(LevelUpSystemTest, XpTableWizardLevel1)
{
    player->playerClassState = Player::PlayerClassState::WIZARD;
    player->set_creature_level(1);
    EXPECT_EQ(player->get_next_level_xp(ctx), 2500);
}

// levelup_update behavior
TEST_F(LevelUpSystemTest, LevelupUpdateBelowThreshold)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_creature_level(1);
    player->destructible->set_xp(1999); // one short

    int levelBefore = player->get_creature_level();
    player->levelup_update(ctx);

    EXPECT_EQ(player->get_creature_level(), levelBefore);
    EXPECT_EQ(player->destructible->get_xp(), 1999);
}

TEST_F(LevelUpSystemTest, LevelupUpdateAtThreshold)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_creature_level(1);
    player->destructible->set_xp(2000); // exactly at threshold

    int levelBefore = player->get_creature_level();
    player->levelup_update(ctx);

    EXPECT_EQ(player->get_creature_level(), levelBefore + 1);
    EXPECT_EQ(player->destructible->get_xp(), 0); // 2000 - 2000
}
