// file: TurnScheduleTest.cpp
// The arithmetic of a time-unit turn order: what an action costs a being of a given
// pace, and which round a point on the clock falls in.
//
// Every expected number is derived from the one definition in TurnSchedule.h - a round
// is TIME_UNITS_PER_ROUND long, and a being whose action delay is that long pays an
// action's base cost unscaled - rather than from running the code. A being's pace is a
// delay in time units, so a bigger number is slower.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=TurnScheduleTest.*

#include <gtest/gtest.h>

#include "src/TurnSchedule.h"

// A normal being pays what the action says and nothing is scaled away.
TEST(TurnScheduleTest, ANormalSpeedPaysTheBaseCostUnchanged)
{
	EXPECT_EQ(scaled_cost(TIME_UNITS_PER_ROUND, TIME_UNITS_PER_ROUND), TIME_UNITS_PER_ROUND);
	EXPECT_EQ(scaled_cost(60, TIME_UNITS_PER_ROUND), 60);
	EXPECT_EQ(scaled_cost(1, TIME_UNITS_PER_ROUND), 1);
}

// Half the delay is half the time for the same action, which is what makes a quickling
// act twice while an ordinary creature acts once.
TEST(TurnScheduleTest, HalfTheDelayHalvesWhatAnActionCosts)
{
	EXPECT_EQ(scaled_cost(TIME_UNITS_PER_ROUND, TIME_UNITS_PER_ROUND / 2), TIME_UNITS_PER_ROUND / 2);
	EXPECT_EQ(scaled_cost(120, 60), 60);
}

// Twice the delay is twice the time, so a snail skips what an ordinary creature does
// not. The same expression covers both directions, which is why this order was chosen.
TEST(TurnScheduleTest, TwiceTheDelayDoublesWhatAnActionCosts)
{
	EXPECT_EQ(scaled_cost(TIME_UNITS_PER_ROUND, TIME_UNITS_PER_ROUND * 2), TIME_UNITS_PER_ROUND * 2);
	EXPECT_EQ(scaled_cost(120, 240), 240);
}

// A base cost is not tied to a round: a move may cost more or less than an action.
TEST(TurnScheduleTest, ABaseCostOtherThanARoundScalesTheSameWay)
{
	EXPECT_EQ(scaled_cost(180, TIME_UNITS_PER_ROUND), 180);
	EXPECT_EQ(scaled_cost(180, TIME_UNITS_PER_ROUND / 3), 60);
	EXPECT_EQ(scaled_cost(80, TIME_UNITS_PER_ROUND), 80);
}

// Pace is monotonic: no increase in delay may ever lower what an action costs. Checked
// across a span rather than at one point, since one point cannot show it.
TEST(TurnScheduleTest, MoreDelayNeverCostsLessTime)
{
	int previous = scaled_cost(TIME_UNITS_PER_ROUND, 1);
	for (int actionDelay = 2; actionDelay <= 400; ++actionDelay)
	{
		const int cost = scaled_cost(TIME_UNITS_PER_ROUND, actionDelay);
		EXPECT_GE(cost, previous) << "delay " << actionDelay << " cost less than delay " << actionDelay - 1;
		previous = cost;
	}
}

// A being that has overshot by a long way still has no actions, and must not come
// back a negative number of them. The ceiling arithmetic alone yields 0 for a small
// overshoot but goes negative for a large one against a cheap action, so the guard
// that refuses an overshot being is load-bearing exactly here.
TEST(TurnScheduleTest, ADeeplyOvershotBeingHasNoActionsRatherThanNegativeOnes)
{
	EXPECT_EQ(actions_before(5000, 10, TIME_UNITS_PER_ROUND), 0);
	EXPECT_EQ(actions_before(TIME_UNITS_PER_ROUND * 50, 1, TIME_UNITS_PER_ROUND), 0);
	EXPECT_EQ(actions_before(1, 1, 0), 0);
}

// Rounds finished and the round in progress are different questions, and the upkeep
// depends on the first: at time zero no round has finished and round 1 is being
// played. Nothing else in the tree distinguishes them, so this is where it is pinned.
TEST(TurnScheduleTest, RoundsFinishedIsNotTheRoundInProgress)
{
	EXPECT_EQ(rounds_completed_at(0), 0);
	EXPECT_EQ(rounds_completed_at(119), 0);
	EXPECT_EQ(rounds_completed_at(TIME_UNITS_PER_ROUND - 1), 0);
	EXPECT_EQ(rounds_completed_at(TIME_UNITS_PER_ROUND), 1);
	EXPECT_EQ(rounds_completed_at(TIME_UNITS_PER_ROUND * 2), 2);
	EXPECT_EQ(rounds_completed_at(300), 2);

	// The two differ by exactly one at every point, which is the whole relation.
	for (int units = 0; units <= TIME_UNITS_PER_ROUND * 5; ++units)
	{
		EXPECT_EQ(round_number_at(units), rounds_completed_at(units) + 1) << "at " << units;
	}
}

// The clock counts time units and a round is a span of them, so the round number is
// read off the clock rather than counted by anything.
TEST(TurnScheduleTest, ARoundIsASpanOfTheClockCountedFromOne)
{
	EXPECT_EQ(round_number_at(0), 1);
	EXPECT_EQ(round_number_at(TIME_UNITS_PER_ROUND - 1), 1);
	EXPECT_EQ(round_number_at(TIME_UNITS_PER_ROUND), 2);
	EXPECT_EQ(round_number_at(TIME_UNITS_PER_ROUND * 2), 3);
	EXPECT_EQ(round_number_at(300), 3);
}

// Every point inside one round reads as that round, which is what lets a duration be
// a timestamp: a buff ending at a time ends in exactly one round.
TEST(TurnScheduleTest, EveryPointInsideARoundReadsAsThatRound)
{
	for (int offset = 0; offset < TIME_UNITS_PER_ROUND; ++offset)
	{
		EXPECT_EQ(round_number_at(TIME_UNITS_PER_ROUND * 4 + offset), 5) << "offset " << offset;
	}
}

// The owner's requirement, stated as arithmetic: one expression has to cover both
// directions. "A player moves, the snail skips, and the quickling moves and attacks
// multiple times." Nothing below branches on fast or slow.
TEST(TurnScheduleTest, AnOrdinaryBeingActsOncePerRound)
{
	EXPECT_EQ(actions_before(0, TIME_UNITS_PER_ROUND, TIME_UNITS_PER_ROUND), 1);
	EXPECT_EQ(actions_before(120, TIME_UNITS_PER_ROUND, 240), 1);
	EXPECT_EQ(actions_before(600, TIME_UNITS_PER_ROUND, 720), 1);
}

// The quickling. Half the cost is two actions in the window, a quarter is four, and
// it chooses what to spend each on - the schedule does not care which.
TEST(TurnScheduleTest, AFasterBeingActsSeveralTimesInOneRound)
{
	const int halfARound = scaled_cost(TIME_UNITS_PER_ROUND, TIME_UNITS_PER_ROUND / 2);
	EXPECT_EQ(actions_before(0, halfARound, TIME_UNITS_PER_ROUND), 2);

	const int quarterOfARound = scaled_cost(TIME_UNITS_PER_ROUND, TIME_UNITS_PER_ROUND / 4);
	EXPECT_EQ(actions_before(0, quarterOfARound, TIME_UNITS_PER_ROUND), 4);
}

// The snail. It acts, overshoots the next limit, and so has no action in the window
// after - which is a skipped turn with nothing anywhere saying "skip".
TEST(TurnScheduleTest, ASlowerBeingSkipsTheWindowItHasOvershot)
{
	const int twoRounds = scaled_cost(TIME_UNITS_PER_ROUND, TIME_UNITS_PER_ROUND * 2);
	ASSERT_EQ(twoRounds, TIME_UNITS_PER_ROUND * 2);

	// It is standing at the start, so it acts once in the first round.
	EXPECT_EQ(actions_before(0, twoRounds, TIME_UNITS_PER_ROUND), 1);

	// That action put it two rounds out, so the second round holds nothing for it.
	EXPECT_EQ(actions_before(twoRounds, twoRounds, TIME_UNITS_PER_ROUND * 2), 0);

	// And the third round holds its next action.
	EXPECT_EQ(actions_before(twoRounds, twoRounds, TIME_UNITS_PER_ROUND * 3), 1);
}

// A being standing exactly on the limit has not reached it yet, so it does not act.
// This is the boundary the snail's skip depends on and it is checked on its own.
TEST(TurnScheduleTest, ABeingStandingOnTheLimitDoesNotActInThatWindow)
{
	EXPECT_EQ(actions_before(120, 120, 120), 0);
	EXPECT_EQ(actions_before(119, 120, 120), 1);
	EXPECT_EQ(actions_before(121, 120, 120), 0);
}

// Over any span, a being acts the number of times its cost divides that span - the
// property the whole order rests on, checked over a range rather than at one point.
TEST(TurnScheduleTest, ActionsInASpanFollowTheCostDividingIt)
{
	for (int rounds = 1; rounds <= 10; ++rounds)
	{
		const int limit = TIME_UNITS_PER_ROUND * rounds;
		EXPECT_EQ(actions_before(0, TIME_UNITS_PER_ROUND, limit), rounds) << rounds << " rounds, normal";
		EXPECT_EQ(actions_before(0, TIME_UNITS_PER_ROUND / 2, limit), rounds * 2) << rounds << " rounds, double";
	}
}

// A duration printed in rounds becomes the clock reading it ends at, so it is a fact
// about the clock rather than a count anything has to maintain.
TEST(TurnScheduleTest, ADurationBecomesTheReadingItEndsAt)
{
	EXPECT_EQ(expiry_time(0, 3), TIME_UNITS_PER_ROUND * 3);
	EXPECT_EQ(expiry_time(TIME_UNITS_PER_ROUND * 5, 3), TIME_UNITS_PER_ROUND * 8)
		<< "a duration granted five rounds in ends three rounds after that";
}

// A duration of no rounds is over at the reading it was granted, which is what a spell
// with nothing left should be rather than one that lasts a round by accident.
TEST(TurnScheduleTest, ADurationOfNoRoundsIsAlreadyOver)
{
	const int granted = TIME_UNITS_PER_ROUND * 4;

	EXPECT_EQ(expiry_time(granted, 0), granted);
	EXPECT_EQ(rounds_remaining(granted, expiry_time(granted, 0)), 0);
}

// What a duration has left, counted in the rounds the book prints it in.
TEST(TurnScheduleTest, RoundsRemainingCountsDownAsTheClockRuns)
{
	const int expiry = expiry_time(0, 3);

	EXPECT_EQ(rounds_remaining(0, expiry), 3);
	EXPECT_EQ(rounds_remaining(TIME_UNITS_PER_ROUND, expiry), 2);
	EXPECT_EQ(rounds_remaining(TIME_UNITS_PER_ROUND * 2, expiry), 1);
	EXPECT_EQ(rounds_remaining(TIME_UNITS_PER_ROUND * 3, expiry), 0);
}

// The invariant that makes the count safe to print: zero means over, and nothing else
// does. A buff with part of a round left has to report one, because a reader that sees
// zero reads it as expired - which is what the count meant before it was a reading.
TEST(TurnScheduleTest, RoundsRemainingReadsZeroOnlyOnceTheDurationIsOver)
{
	const int expiry = expiry_time(0, 3);

	for (int currentTime = 0; currentTime < expiry; ++currentTime)
	{
		ASSERT_GT(rounds_remaining(currentTime, expiry), 0) << "still in force at " << currentTime;
	}

	EXPECT_EQ(rounds_remaining(expiry, expiry), 0);

	// Two rounds past, not one. Without the guard the ceiling arithmetic returns a
	// negative count, and one round past is the last overshoot it still truncates to
	// zero - so a case measured there cannot see the guard at all.
	EXPECT_EQ(rounds_remaining(expiry + TIME_UNITS_PER_ROUND * 2, expiry), 0) << "and stays zero past it";
}

// Half a round left is still a round the creature has, which is the case a duration
// measured in whole rounds cannot see and a fast creature produces constantly.
TEST(TurnScheduleTest, APartialRoundLeftStillReadsAsOne)
{
	EXPECT_EQ(rounds_remaining(TIME_UNITS_PER_ROUND / 2, TIME_UNITS_PER_ROUND), 1);
	EXPECT_EQ(rounds_remaining(TIME_UNITS_PER_ROUND - 1, TIME_UNITS_PER_ROUND), 1);
}

// A delay of one round costs one round, and a bigger delay costs more. That is what
// measuring pace in time units costs - as a percentage, a bigger number meant faster -
// so the direction gets a case of its own rather than living in the names of the
// creatures that use it elsewhere.
TEST(TurnScheduleTest, ABiggerDelayCostsMoreTime)
{
	const int ordinary = scaled_cost(TIME_UNITS_PER_ROUND, TIME_UNITS_PER_ROUND);
	const int halfPace = scaled_cost(TIME_UNITS_PER_ROUND, TIME_UNITS_PER_ROUND * 2);
	const int doublePace = scaled_cost(TIME_UNITS_PER_ROUND, TIME_UNITS_PER_ROUND / 2);

	EXPECT_EQ(ordinary, TIME_UNITS_PER_ROUND) << "a delay of one round did not cost one round";
	EXPECT_GT(halfPace, ordinary) << "twice the delay did not take longer";
	EXPECT_LT(doublePace, ordinary) << "half the delay did not act sooner";
}
