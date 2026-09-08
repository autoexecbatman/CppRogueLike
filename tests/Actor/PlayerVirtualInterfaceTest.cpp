#include "src/Actor/Actor.h"
#include "src/Actor/Creature.h"
#include "src/ActorTypes/Player.h"
#include "src/Systems/DataManager.h"
#include "tests/mocks/MockGameContext.h"
#include <Core/GameContext.h>
#include <gtest/gtest.h>
#include <memory>
#include <Utils/Vector2D.h>
#include "src/Combat/ExperienceReward.h"

// ============================================================================
// PLAYER VIRTUAL INTERFACE TESTS
// Guards the Creature* ctx.player() contract introduced when Player* was widened
// to Creature*. Any regression here means a cast crept back in somewhere.
// ============================================================================

class PlayerVirtualInterfaceTest : public ::testing::Test
{
protected:
    MockGameContext mock;
    GameContext ctx;
    DataManager data_manager;
    std::unique_ptr<Player> player;
    std::unique_ptr<Creature> creature_base;

    void SetUp() override
    {
        data_manager.load_all_data(mock.messages);

        player = std::make_unique<Player>(Vector2D{ 0, 0 });
        player->experienceReward = std::make_unique<ExperienceReward>(0);
        player->set_dr(5);
        player->set_thaco(20);
        player->armorClass = std::make_unique<ArmorClass>(10);
        player->healthPool = std::make_unique<HealthPool>(100);

        creature_base = std::make_unique<Creature>(
            Vector2D{ 1, 1 },
            ActorData{ TileRef{}, "test_creature", 1 });
        creature_base->experienceReward = std::make_unique<ExperienceReward>(50);
        creature_base->set_dr(2);
        creature_base->set_thaco(19);
        creature_base->armorClass = std::make_unique<ArmorClass>(7);
        creature_base->healthPool = std::make_unique<HealthPool>(30);

        ctx = mock.to_game_context();
        ctx.playerOwner = &player;
    }
};

// ----------------------------------------------------------------------------
// Player display accessors — read directly off the concrete Player.
// ----------------------------------------------------------------------------

TEST_F(PlayerVirtualInterfaceTest, Player_GetClassDisplayName_ReturnsPlayerClass)
{
    player->playerClass = "Fighter";

    EXPECT_EQ(player->get_class_display_name(), "Fighter");
}

TEST_F(PlayerVirtualInterfaceTest, Player_GetRaceDisplayName_ReturnsPlayerRace)
{
    player->playerRace = "Elf";

    EXPECT_EQ(player->get_race_display_name(), "Elf");
}

TEST_F(PlayerVirtualInterfaceTest, Player_GetKillCount_ReturnsKillCount)
{
    player->killCount = 7;

    EXPECT_EQ(player->get_kill_count(), 7);
}

// ----------------------------------------------------------------------------
// on_kill_reward — critical path (replaced 3 lines of direct Player mutation)
// Regression here means Creature::die() is broken.
// ----------------------------------------------------------------------------

TEST_F(PlayerVirtualInterfaceTest, OnKillReward_XpAddedToDestructible)
{
    int xpBefore = player->get_xp();

    player->on_kill_reward(150, ctx);

    EXPECT_EQ(player->get_xp(), xpBefore + 150);
}

TEST_F(PlayerVirtualInterfaceTest, OnKillReward_KillCountIncrements)
{
    int killsBefore = player->get_kill_count();

    player->on_kill_reward(100, ctx);

    EXPECT_EQ(player->get_kill_count(), killsBefore + 1);
}

TEST_F(PlayerVirtualInterfaceTest, OnKillReward_ZeroXp_KillCountStillIncrements)
{
    int killsBefore = player->get_kill_count();
    int xpBefore = player->get_xp();

    player->on_kill_reward(0, ctx);

    EXPECT_EQ(player->get_kill_count(), killsBefore + 1);
    EXPECT_EQ(player->get_xp(), xpBefore);
}

TEST_F(PlayerVirtualInterfaceTest, OnKillReward_MultipleRewards_Accumulate)
{
    int xpBefore = player->get_xp();

    player->on_kill_reward(100, ctx);
    player->on_kill_reward(200, ctx);
    player->on_kill_reward(50, ctx);

    EXPECT_EQ(player->get_xp(), xpBefore + 350);
    EXPECT_EQ(player->get_kill_count(), 3);
}
