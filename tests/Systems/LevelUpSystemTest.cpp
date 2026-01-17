#include <gtest/gtest.h>
#include "src/Systems/LevelUpSystem.h"
#include "src/ActorTypes/Player.h"
#include "src/Core/GameContext.h"
#include "src/Actor/Destructible.h"
#include "src/Systems/DataManager.h"
#include "src/Systems/MessageSystem.h"
#include "src/Random/RandomDice.h"

class LevelUpSystemTest : public ::testing::Test
{
protected:
    std::unique_ptr<Player> player;
    GameContext ctx;
    DataManager data_manager;
    MessageSystem message_system;
    RandomDice dice;

    void SetUp() override
    {
        // Initialize game data
        // We need to load data for constitution bonuses etc.
        // Assuming JSON files are present in the test execution directory (handled by CMake)
        try
        {
            data_manager.load_all_data(message_system);
        }
        catch (...)
        {
            // Ignore errors if files are missing, tests will just run without bonuses
        }

        player = std::make_unique<Player>(Vector2D{0, 0});
        
        // Initialize destructible component which is required by LevelUpSystem
        // Player constructor doesn't initialize it (it waits for roll_new_character)
        player->destructible = std::make_unique<PlayerDestructible>(
            10, 0, "your corpse", 0, 0, 10
        );
        player->destructible->set_hp_base(10);
        
        // Setup GameContext
        ctx.player = player.get();
        ctx.data_manager = &data_manager;
        ctx.message_system = &message_system;
        
        // Enable testing mode for dice to have deterministic results
        dice.set_test_mode(true);
    }

    void TearDown() override {
        dice.set_test_mode(false);
        dice.clear_fixed_rolls();
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
    // Setup player as Fighter
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_constitution(10); // Average constitution, no bonus
    
    // Initial stats
    player->destructible->set_hp_base(10);
    player->destructible->set_max_hp(10);
    player->destructible->set_hp(10);

    // Predictable dice roll: 8
    dice.set_next_d20(8);

    // Apply level up
    LevelUpSystem::apply_level_up_benefits(*player, 2, &ctx);

    // Check HP gain
    // Fighter uses d10. Rolled 8. Con bonus 0. Total gain 8.
    EXPECT_EQ(player->destructible->get_hp_base(), 18);
    EXPECT_EQ(player->destructible->get_max_hp(), 18);
    EXPECT_EQ(player->destructible->get_hp(), 18);
}

TEST_F(LevelUpSystemTest, WizardLevelUpHPGainWithConBonus)
{
    // Setup player as Wizard
    player->playerClassState = Player::PlayerClassState::WIZARD;
    
    // High constitution (16) should give +2 bonus (standard AD&D)
    // We rely on DataManager loading the correct json. 
    // If json is not loaded, bonus is 0. 
    // Let's verify if data is loaded first or just set expectation conditionally?
    // Better: set constitution to a value we know has a bonus if data loaded.
    player->set_constitution(16);
    
    // Check if we have data loaded to know what to expect
    int expectedBonus = 0;
    if (!data_manager.get_constitution_attributes().empty()) {
        if (player->get_constitution() <= data_manager.get_constitution_attributes().size()) {
            expectedBonus = data_manager.get_constitution_attributes()[player->get_constitution() - 1].HPAdj;
        }
    }

    // Initial stats
    player->destructible->set_hp_base(4);
    player->destructible->set_max_hp(4 + expectedBonus);
    player->destructible->set_hp(4 + expectedBonus);
    
    int oldMaxHP = player->destructible->get_max_hp();

    // Predictable dice roll: 3
    dice.set_next_d20(3);

    // Apply level up
    LevelUpSystem::apply_level_up_benefits(*player, 2, &ctx);

    // Check HP gain
    // Wizard uses d4. Rolled 3. Con bonus expectedBonus.
    int gained = 3 + expectedBonus;
    EXPECT_EQ(player->destructible->get_max_hp(), oldMaxHP + gained);
}

TEST_F(LevelUpSystemTest, FighterExtraAttackAtLevel7)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->attacksPerRound = 1.0f;

    // Apply level up to 7
    LevelUpSystem::apply_level_up_benefits(*player, 7, &ctx);

    EXPECT_FLOAT_EQ(player->attacksPerRound, 1.5f);
}

TEST_F(LevelUpSystemTest, FighterExtraAttackAtLevel13)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->attacksPerRound = 1.5f;

    // Apply level up to 13
    LevelUpSystem::apply_level_up_benefits(*player, 13, &ctx);

    EXPECT_FLOAT_EQ(player->attacksPerRound, 2.0f);
}

TEST_F(LevelUpSystemTest, AbilityScoreImprovement)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_strength(15);
    
    // Apply level up to 4 (divisible by 4)
    LevelUpSystem::apply_level_up_benefits(*player, 4, &ctx);
    
    EXPECT_EQ(player->get_strength(), 16);
}

TEST_F(LevelUpSystemTest, NoAbilityScoreImprovementAtInterimLevel)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->set_strength(15);
    
    // Apply level up to 5
    LevelUpSystem::apply_level_up_benefits(*player, 5, &ctx);
    
    EXPECT_EQ(player->get_strength(), 15);
}

TEST_F(LevelUpSystemTest, FighterTHAC0Improvement)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    
    // Level 1 Fighter THAC0 is 20
    player->destructible->set_thaco(20);
    
    // Apply level up to 2 (Fighter THAC0 becomes 19)
    // We need to consume the dice rolls for HP calculation (1 roll for d10)
    dice.set_next_d20(5); 
    
    LevelUpSystem::apply_level_up_benefits(*player, 2, &ctx);
    
    EXPECT_EQ(player->destructible->get_thaco(), 19);
}

TEST_F(LevelUpSystemTest, FighterExtraAttackSkippedLevel)
{
    player->playerClassState = Player::PlayerClassState::FIGHTER;
    player->attacksPerRound = 1.0f;
    
    // Jump from level 6 to 8 (skipping 7)
    // Needs dice roll for HP
    dice.set_next_d20(5);

    LevelUpSystem::apply_level_up_benefits(*player, 8, &ctx);
    
    // Should still get the 1.5 attacks benefit
    EXPECT_FLOAT_EQ(player->attacksPerRound, 1.5f);
}
