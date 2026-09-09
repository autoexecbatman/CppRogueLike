// file: WebTest.cpp
// Verifies that webs catch any creature without CAN_WALK_WEBS, that creatures
// with that state pass freely, and that a caught creature struggles for a turn
// before it is released.

#include <gtest/gtest.h>

#include "src/Actor/Creature.h"
#include "src/ActorTypes/Player.h"
#include "src/Objects/Web.h"
#include "src/Utils/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

class WebTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->set_dexterity(10);
		player->set_strength(10);
	}

	// Rolls are consumed in order by RandomDice's fixed-roll queue, so each
	// test states exactly the rolls the code under test will draw.
	void force_next_roll(int value)
	{
		mock.dice.set_next_roll(value);
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 5, 5 }) };
	Web web{ Vector2D{ 5, 5 }, 2, mock.tile_config };
};

// A creature that cannot walk webs is caught and its move is stopped.
TEST_F(WebTest, CatchesCreatureWithoutWebWalking)
{
	force_next_roll(1); // caught
	force_next_roll(1); // stuck-turn roll

	EXPECT_EQ(web.on_creature_enter(*player, ctx), EntryResult::BLOCKED);
	EXPECT_TRUE(player->is_webbed());
}

// A spider walks its own webs: nothing happens and the web is untouched.
TEST_F(WebTest, CreatureThatWalksWebsIsUnaffected)
{
	player->add_state(ActorState::CAN_WALK_WEBS);

	EXPECT_EQ(web.on_creature_enter(*player, ctx), EntryResult::UNAFFECTED);
	EXPECT_FALSE(player->is_webbed());
	EXPECT_FALSE(web.is_destroyed());
}

// A failed catch roll still counts as an effect, and never binds the creature.
TEST_F(WebTest, FailedCatchLeavesCreatureFree)
{
	force_next_roll(100); // escapes the catch
	force_next_roll(2); // web survives the tear roll

	EXPECT_EQ(web.on_creature_enter(*player, ctx), EntryResult::AFFECTED);
	EXPECT_FALSE(player->is_webbed());
}

// A held creature that fails its break roll stays stuck and spends the turn.
TEST_F(WebTest, FailedBreakKeepsCreatureStuck)
{
	player->apply_web_effect(3, 2, nullptr);

	force_next_roll(100); // fails to break free

	EXPECT_EQ(player->try_break_web(ctx), WebEscape::STILL_STUCK);
	EXPECT_TRUE(player->is_webbed());
}

// A successful break roll frees the creature immediately.
TEST_F(WebTest, SuccessfulBreakFreesCreature)
{
	player->apply_web_effect(3, 2, nullptr);

	force_next_roll(1); // breaks free

	EXPECT_EQ(player->try_break_web(ctx), WebEscape::BROKE_FREE);
	EXPECT_FALSE(player->is_webbed());
}

// The binding expires on its own even when every break roll fails.
TEST_F(WebTest, BindingExpiresAfterItsDuration)
{
	player->apply_web_effect(1, 2, nullptr);

	force_next_roll(100); // fails to break, but the last turn runs out

	EXPECT_EQ(player->try_break_web(ctx), WebEscape::STRUGGLED_FREE);
	EXPECT_FALSE(player->is_webbed());
}
