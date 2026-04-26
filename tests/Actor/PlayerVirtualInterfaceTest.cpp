#include "src/Actor/Actor.h"
#include "src/Actor/Creature.h"
#include "src/Actor/Destructible.h"
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
// Guards the Creature* ctx.player contract introduced when Player* was widened
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
        player->destructible = std::make_unique<Destructible>(100, 10, std::make_unique<PlayerDeathHandler>());

        creature_base = std::make_unique<Creature>(
            Vector2D{ 1, 1 },
            ActorData{ TileRef{}, "test_creature", 1 });
        creature_base->experienceReward = std::make_unique<ExperienceReward>(50);
        creature_base->set_dr(2);
        creature_base->set_thaco(19);
        creature_base->armorClass = std::make_unique<ArmorClass>(7);
        creature_base->destructible = std::make_unique<Destructible>(30, 7, std::make_unique<MonsterDeathHandler>());

        ctx = mock.to_game_context();
        ctx.player = player.get();
    }
};

// ----------------------------------------------------------------------------
// Creature base class — virtual default sentinels
// If these return wrong values the hierarchy contract is broken.
// ----------------------------------------------------------------------------

TEST_F(PlayerVirtualInterfaceTest, CreatureBase_GetClassDisplayName_ReturnsEmpty)
{
    EXPECT_EQ(creature_base->get_class_display_name(), std::string{});
}

TEST_F(PlayerVirtualInterfaceTest, CreatureBase_GetRaceDisplayName_ReturnsEmpty)
{
    EXPECT_EQ(creature_base->get_race_display_name(), std::string{});
}

TEST_F(PlayerVirtualInterfaceTest, CreatureBase_GetKillCount_ReturnsZero)
{
    EXPECT_EQ(creature_base->get_kill_count(), 0);
}

TEST_F(PlayerVirtualInterfaceTest, CreatureBase_GetEquippedWeaponDamageRoll_ReturnsQuestionMark)
{
    EXPECT_EQ(creature_base->get_equipped_weapon_damage_roll(), "?");
}

// ----------------------------------------------------------------------------
// Player overrides — correct values dispatched through Creature*
// These prove the virtual table routes to Player without any cast.
// ----------------------------------------------------------------------------

TEST_F(PlayerVirtualInterfaceTest, Player_GetClassDisplayName_ReturnsPlayerClass)
{
    player->playerClass = "Fighter";

    Creature* asCreature = player.get();
    EXPECT_EQ(asCreature->get_class_display_name(), "Fighter");
}

TEST_F(PlayerVirtualInterfaceTest, Player_GetRaceDisplayName_ReturnsPlayerRace)
{
    player->playerRace = "Elf";

    Creature* asCreature = player.get();
    EXPECT_EQ(asCreature->get_race_display_name(), "Elf");
}

TEST_F(PlayerVirtualInterfaceTest, Player_GetKillCount_ReturnsKillCount)
{
    player->killCount = 7;

    Creature* asCreature = player.get();
    EXPECT_EQ(asCreature->get_kill_count(), 7);
}

// ----------------------------------------------------------------------------
// on_kill_reward — critical path (replaced 3 lines of direct Player mutation)
// Regression here means MonsterDeathHandler::execute is broken.
// ----------------------------------------------------------------------------

TEST_F(PlayerVirtualInterfaceTest, OnKillReward_XpAddedToDestructible)
{
    int xpBefore = player->get_xp();

    ctx.player->on_kill_reward(150, ctx);

    EXPECT_EQ(player->get_xp(), xpBefore + 150);
}

TEST_F(PlayerVirtualInterfaceTest, OnKillReward_KillCountIncrements)
{
    int killsBefore = player->get_kill_count();

    ctx.player->on_kill_reward(100, ctx);

    EXPECT_EQ(player->get_kill_count(), killsBefore + 1);
}

TEST_F(PlayerVirtualInterfaceTest, OnKillReward_ZeroXp_KillCountStillIncrements)
{
    int killsBefore = player->get_kill_count();
    int xpBefore = player->get_xp();

    ctx.player->on_kill_reward(0, ctx);

    EXPECT_EQ(player->get_kill_count(), killsBefore + 1);
    EXPECT_EQ(player->get_xp(), xpBefore);
}

TEST_F(PlayerVirtualInterfaceTest, OnKillReward_MultipleRewards_Accumulate)
{
    int xpBefore = player->get_xp();

    ctx.player->on_kill_reward(100, ctx);
    ctx.player->on_kill_reward(200, ctx);
    ctx.player->on_kill_reward(50, ctx);

    EXPECT_EQ(player->get_xp(), xpBefore + 350);
    EXPECT_EQ(player->get_kill_count(), 3);
}

TEST_F(PlayerVirtualInterfaceTest, OnKillReward_DispatchedThroughCreaturePtr)
{
    // The entire point of the refactor: ctx.player is Creature*, no cast needed.
    // This test calls on_kill_reward through Creature* and verifies Player state.
    Creature* asCreature = player.get();
    int xpBefore = player->get_xp();

    asCreature->on_kill_reward(300, ctx);

    EXPECT_EQ(player->get_xp(), xpBefore + 300);
    EXPECT_EQ(player->killCount, 1);
}
