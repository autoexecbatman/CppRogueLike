// file: PlayerActionCostTest.cpp
// The one place that says the player spent an action.
//
// Thirty-one sites across seven files used to set the game status to NEW_TURN by
// hand, which could say that a turn was used and nothing about how much of one.
// spend_player_action says both: it moves the player along the shared clock by what
// the action cost, and then opens the turn.
//
// What is checked here is that the charge is additive, that it is scaled by the
// player's own action delay, and that the turn still opens - because every one of those
// call sites depends on the last of those and nothing else.
//
// The clock is the player's own place on it: the game advances when the player acts
// and never otherwise, so there is one number rather than two.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=PlayerActionCostTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/Game.h"
#include "src/Player.h"
#include "src/PlayerTurn.h"
#include "src/TurnSchedule.h"

class PlayerActionCostTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = game.context();
		ctx.playerOwner = &player;
		ctx.gameState->set_game_status(GameStatus::IDLE);
	}

	Game game;
	GameContext ctx;
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 0, 0 }) };
};

// An ordinary player pays an action's stated cost, and the turn opens. The second
// half is what every call site was written for and it must not be lost.
TEST_F(PlayerActionCostTest, AnOrdinaryActionCostsItsStatedTimeAndOpensTheTurn)
{
	ASSERT_EQ(ctx.gameState->get_time(), 0);

	spend_player_action(ctx, TIME_UNITS_PER_ROUND);

	EXPECT_EQ(ctx.gameState->get_time(), TIME_UNITS_PER_ROUND);
	EXPECT_EQ(ctx.gameState->get_game_status(), GameStatus::NEW_TURN);
}

// The charge accumulates. Under the status enum two calls were the same as one,
// which is why a redundant second call sat unnoticed in the trap path until the
// clock made it cost something.
TEST_F(PlayerActionCostTest, ActionsAccumulateRatherThanOverwriting)
{
	spend_player_action(ctx, TIME_UNITS_PER_ROUND);
	spend_player_action(ctx, TIME_UNITS_PER_ROUND);
	spend_player_action(ctx, TIME_UNITS_PER_ROUND);

	EXPECT_EQ(ctx.gameState->get_time(), TIME_UNITS_PER_ROUND * 3);
}

// A cheaper action costs less of the clock, which is the whole reason the cost is a
// parameter rather than a constant inside the function.
TEST_F(PlayerActionCostTest, ACheaperActionTakesLessOfTheClock)
{
	spend_player_action(ctx, TIME_UNITS_PER_ROUND / 2);

	EXPECT_EQ(ctx.gameState->get_time(), TIME_UNITS_PER_ROUND / 2);
}

// The player's own action delay prices what it is charged, so a hasted player arrives at
// its next action sooner having done the same thing.
TEST_F(PlayerActionCostTest, ThePlayersActionDelayPricesWhatAnActionCosts)
{
	player->set_action_delay(TIME_UNITS_PER_ROUND / 2);

	spend_player_action(ctx, TIME_UNITS_PER_ROUND);

	EXPECT_EQ(ctx.gameState->get_time(), TIME_UNITS_PER_ROUND / 2)
		<< "half the delay should charge half the time";
}

// And a slower player pays more, from the same expression and no second branch.
TEST_F(PlayerActionCostTest, ASlowedPlayerPaysMoreForTheSameAction)
{
	player->set_action_delay(TIME_UNITS_PER_ROUND * 2);

	spend_player_action(ctx, TIME_UNITS_PER_ROUND);

	EXPECT_EQ(ctx.gameState->get_time(), TIME_UNITS_PER_ROUND * 2);
}
