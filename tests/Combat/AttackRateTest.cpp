// file: AttackRateTest.cpp
// What an attack rate costs on the clock, against Table 58.
//
// The book gives a warrior 3/2 attacks from 7th level and 2 from 13th (Player's
// Handbook, PDF page 231). Under a time-unit turn order a rate is spent as a price:
// an attack costing two thirds of a round pays out three attacks every two rounds by
// itself, with no round having to be the odd one.
//
// The defect this replaces: the alternation was keyed on a counter that advanced only
// when the player struck, so the extra half-attack banked across every round spent
// walking. A cost cannot bank - it is paid when the attack happens.
//
// Every expected number is derived from TIME_UNITS_PER_ROUND divided by the rate,
// and every rate's payout is checked against the book's own words rather than
// against what the code returns.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=AttackRateTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/AttackRate.h"
#include "src/Game.h"
#include "src/Player.h"
#include "src/PlayerController.h"
#include "src/PlayerTurn.h"
#include "src/TurnSchedule.h"

// One attack a round costs a whole round, which is the floor the book prints for a
// character and the cost every creature has until something raises its rate.
TEST(AttackRateTest, ABaseRateCostsAWholeRound)
{
	EXPECT_EQ(attack_cost(1.0f), TIME_UNITS_PER_ROUND);
}

// Two a round costs half a round each, which is 13th level.
TEST(AttackRateTest, ADoubleRateCostsHalfARound)
{
	EXPECT_EQ(attack_cost(2.0f), TIME_UNITS_PER_ROUND / 2);
}

// Three attacks in two rounds costs two thirds of a round each, which is 7th level.
TEST(AttackRateTest, AThreeHalvesRateCostsTwoThirdsOfARound)
{
	EXPECT_EQ(attack_cost(1.5f), TIME_UNITS_PER_ROUND * 2 / 3);
}

// The property the costs exist to satisfy, checked against the book's own sentence:
// "three attacks every two rounds". Counted off the schedule rather than asserted.
TEST(AttackRateTest, EachRatePaysOutWhatTheBookPrintsOverTwoRounds)
{
	const int twoRounds = TIME_UNITS_PER_ROUND * 2;

	EXPECT_EQ(actions_before(0, attack_cost(1.0f), twoRounds), 2) << "one a round";
	EXPECT_EQ(actions_before(0, attack_cost(1.5f), twoRounds), 3) << "three in two rounds";
	EXPECT_EQ(actions_before(0, attack_cost(2.0f), twoRounds), 4) << "two a round";
}

// And over a longer span, so a rate that is slightly wrong per round shows up as a
// drift rather than hiding inside one window.
TEST(AttackRateTest, TheThreeHalvesRateDoesNotDriftOverTenRounds)
{
	const int tenRounds = TIME_UNITS_PER_ROUND * 10;

	EXPECT_EQ(actions_before(0, attack_cost(1.5f), tenRounds), 15);
	EXPECT_EQ(actions_before(0, attack_cost(2.0f), tenRounds), 20);
	EXPECT_EQ(actions_before(0, attack_cost(1.0f), tenRounds), 10);
}

// A rate the book never printed is a cost like any other rather than a case to
// handle, which is what replacing the alternation bought.
TEST(AttackRateTest, ARateTheBookNeverPrintedIsStillACost)
{
	EXPECT_EQ(attack_cost(3.0f), TIME_UNITS_PER_ROUND / 3);
	EXPECT_EQ(attack_cost(4.0f), TIME_UNITS_PER_ROUND / 4);

	// Slower than once a round costs more than a round, from the same expression.
	EXPECT_EQ(attack_cost(0.5f), TIME_UNITS_PER_ROUND * 2);
}

// An attack can never be free, or a creature would swing without the clock moving.
TEST(AttackRateTest, AnAttackAlwaysCostsAtLeastOneTimeUnit)
{
	EXPECT_GE(attack_cost(1000.0f), 1);
	EXPECT_GE(attack_cost(100000.0f), 1);
}

// The wiring: a swing moves the clock by what the rate prices it at, so a faster
// character comes round again sooner rather than swinging twice in one go.
class AttackChargeTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = game.context();
		ctx.playerOwner = &player;
	}

	Game game;
	GameContext ctx;
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 0, 0 }) };
};

// A 7th-level fighter's swing takes two thirds of a round off the clock, where an
// ordinary character's takes a whole one.
TEST_F(AttackChargeTest, AFasterAttackerSpendsLessOfTheClockPerSwing)
{
	player->set_attacks_per_round(1.0f);
	const int before = ctx.gameState->get_time();
	spend_player_action(ctx, attack_cost(player->get_attacks_per_round()));
	const int ordinary = ctx.gameState->get_time() - before;

	player->set_attacks_per_round(1.5f);
	const int middle = ctx.gameState->get_time();
	spend_player_action(ctx, attack_cost(player->get_attacks_per_round()));
	const int faster = ctx.gameState->get_time() - middle;

	EXPECT_EQ(ordinary, TIME_UNITS_PER_ROUND);
	EXPECT_EQ(faster, TIME_UNITS_PER_ROUND * 2 / 3);
	EXPECT_LT(faster, ordinary);
}
