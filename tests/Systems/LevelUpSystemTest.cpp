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
    player->attacksPerRound = 1.0f;

    LevelUpSystem::apply_level_up_benefits(*player, 7, &ctx);

    EXPECT_FLOAT_EQ(player->attacksPerRound, 1.5f);
}

TEST_F(LevelUpSystemTest, FighterExtraAttackAtLevel13)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->attacksPerRound = 1.5f;

    LevelUpSystem::apply_level_up_benefits(*player, 13, &ctx);

    EXPECT_FLOAT_EQ(player->attacksPerRound, 2.0f);
}

TEST_F(LevelUpSystemTest, AbilityScoreImprovement)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_strength(15);

    LevelUpSystem::apply_level_up_benefits(*player, 4, &ctx);

    EXPECT_EQ(player->get_strength(), 16);
}

TEST_F(LevelUpSystemTest, NoAbilityScoreImprovementAtInterimLevel)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_strength(15);

    LevelUpSystem::apply_level_up_benefits(*player, 5, &ctx);

    EXPECT_EQ(player->get_strength(), 15);
}

TEST_F(LevelUpSystemTest, FighterTHAC0Improvement)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->destructible->set_thaco(20);

    game.dice.set_next_d20(5);

    LevelUpSystem::apply_level_up_benefits(*player, 2, &ctx);

    EXPECT_EQ(player->destructible->get_thaco(), 19);
}

TEST_F(LevelUpSystemTest, FighterExtraAttackSkippedLevel)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->attacksPerRound = 1.0f;

    game.dice.set_next_d20(5);

    LevelUpSystem::apply_level_up_benefits(*player, 8, &ctx);

    EXPECT_FLOAT_EQ(player->attacksPerRound, 1.5f);
}
