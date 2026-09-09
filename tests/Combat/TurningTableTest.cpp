// file: TurningTableTest.cpp
// Checks the turning table against AD&D 2e Table 61 (Player's Handbook page
// 208-209), including the worked example the book prints beneath it.
//
// Expected values are read from the book, never from the table under test.

#include <gtest/gtest.h>

#include "src/Combat/TurningTable.h"

// The book's own example: Gorus, a 7th-level priest, rolls 12 against two
// skeletons led by a wight and a spectre. "The skeletons are destroyed... The
// wight is turned (a 4 or better was needed) and flees. The spectre, however,
// continues forward undaunted (since a 16 was needed)."
TEST(TurningTableTest, ReproducesTheBooksWorkedExample)
{
	constexpr int gorusLevel = 7;
	constexpr int roll = 12;

	const TurningResult skeleton = look_up_turning(1, gorusLevel);
	EXPECT_EQ(skeleton.outcome, TurningOutcome::DESTROYED) << "skeletons are destroyed";

	const TurningResult wight = look_up_turning(5, gorusLevel);
	EXPECT_EQ(wight.outcome, TurningOutcome::ROLL_REQUIRED);
	EXPECT_EQ(wight.rollNeeded, 4) << "the book says a 4 or better was needed";
	EXPECT_GE(roll, wight.rollNeeded) << "so Gorus turns the wight";

	const TurningResult spectre = look_up_turning(8, gorusLevel);
	EXPECT_EQ(spectre.outcome, TurningOutcome::ROLL_REQUIRED);
	EXPECT_EQ(spectre.rollNeeded, 16) << "the book says a 16 was needed";
	EXPECT_LT(roll, spectre.rollNeeded) << "so the spectre is undaunted";
}

// A first-level priest against the weakest undead: the table's top-left corner.
TEST(TurningTableTest, NovicePriestNeedsTenAgainstASkeleton)
{
	const TurningResult result = look_up_turning(1, 1);

	EXPECT_EQ(result.outcome, TurningOutcome::ROLL_REQUIRED);
	EXPECT_EQ(result.rollNeeded, 10);
}

// A dash in the book means the attempt is not possible at all.
TEST(TurningTableTest, WeakPriestCannotTouchPowerfulUndead)
{
	EXPECT_EQ(look_up_turning(11, 1).outcome, TurningOutcome::BEYOND_POWER) << "lich vs level 1";
	EXPECT_EQ(look_up_turning(6, 2).outcome, TurningOutcome::BEYOND_POWER) << "6 HD vs level 2";
}

// Turning improves monotonically with level: a priest never does worse against
// the same undead by gaining a level.
TEST(TurningTableTest, TurningNeverWorsensWithLevel)
{
	// Ordered from least to most effective, so a later column never ranks lower.
	const auto rank = [](TurningResult result)
	{
		switch (result.outcome)
		{
		case TurningOutcome::BEYOND_POWER:
		{
			return 0;
		}
		case TurningOutcome::ROLL_REQUIRED:
		{
			// A lower target is better, so rank rises as the target falls.
			return 21 - result.rollNeeded;
		}
		case TurningOutcome::ALWAYS_TURNED:
		{
			return 100;
		}
		case TurningOutcome::DESTROYED:
		{
			return 200;
		}
		}
		return 0;
	};

	for (int hitDice = TURNING_MIN_HIT_DICE; hitDice <= TURNING_MAX_HIT_DICE; ++hitDice)
	{
		for (int level = 1; level < TURNING_MAX_PRIEST_LEVEL; ++level)
		{
			EXPECT_LE(rank(look_up_turning(hitDice, level)), rank(look_up_turning(hitDice, level + 1)))
				<< "got worse at " << hitDice << " HD going from level " << level << " to " << level + 1;
		}
	}
}

// Tougher undead are never easier to turn at a fixed priest level.
TEST(TurningTableTest, TougherUndeadAreNeverEasier)
{
	for (int level = 1; level <= TURNING_MAX_PRIEST_LEVEL; ++level)
	{
		for (int hitDice = TURNING_MIN_HIT_DICE; hitDice < TURNING_MAX_HIT_DICE; ++hitDice)
		{
			const TurningResult easier = look_up_turning(hitDice, level);
			const TurningResult harder = look_up_turning(hitDice + 1, level);

			if (easier.outcome == TurningOutcome::ROLL_REQUIRED
				&& harder.outcome == TurningOutcome::ROLL_REQUIRED)
			{
				EXPECT_LE(easier.rollNeeded, harder.rollNeeded)
					<< hitDice + 1 << " HD was easier than " << hitDice << " at level " << level;
			}
		}
	}
}

// Beyond the table's edges the book's "11+ HD" and "14+" columns take over.
TEST(TurningTableTest, BeyondTheTableEdgesClamp)
{
	EXPECT_EQ(look_up_turning(40, 14).rollNeeded, look_up_turning(11, 14).rollNeeded)
		<< "a 40 HD undead turns as the hardest row";
	EXPECT_EQ(look_up_turning(11, 99).rollNeeded, look_up_turning(11, 14).rollNeeded)
		<< "a level 99 priest turns as the last column";
	EXPECT_EQ(look_up_turning(1, 0).outcome, TurningOutcome::BEYOND_POWER)
		<< "level 0 can turn nothing";
}
