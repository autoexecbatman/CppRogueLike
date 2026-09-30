// file: TurningTableTest.cpp
// Checks the turning table against AD&D 2e Table 61 (Player's Handbook page
// 208-209), including the worked example the book prints beneath it.
//
// Expected values are read from the book, never from the table under test. The
// page is transcribed below in the book's own twelve columns, three of which
// cover more than one priest level - so the test knows which levels a column
// covers rather than assuming one column is one level.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=TurningTableTest.*

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>

#include "src/TurningTable.h"

namespace
{
// Table 61's columns as the book heads them: "1" through "9", then "10-11",
// "12-13" and "14+".
constexpr int PRINTED_COLUMNS = 12;

// The priest levels one printed column covers. The three that cover two levels
// are the reason this exists.
struct ColumnSpan
{
	int firstLevel{ 0 };
	int lastLevel{ 0 };
};

constexpr std::array<ColumnSpan, PRINTED_COLUMNS> COLUMN_SPANS = { {
	{ 1, 1 },
	{ 2, 2 },
	{ 3, 3 },
	{ 4, 4 },
	{ 5, 5 },
	{ 6, 6 },
	{ 7, 7 },
	{ 8, 8 },
	{ 9, 9 },
	{ 10, 11 },
	{ 12, 13 },
	{ 14, 14 },
} };

struct PrintedRow
{
	int hitDice{ 0 };
	std::array<std::string_view, PRINTED_COLUMNS> cells{};
};

// Transcribed from the page. The book's "D*" turns an extra 2d4 creatures of the
// type, which the lookup does not distinguish, so it is written here as the "D"
// it is read as. Its "Zombie", "Ghast" and "Special" rows name a creature rather
// than a number and have no place in a table indexed by hit dice.
constexpr std::array<PrintedRow, 11> PRINTED_TABLE = { {
	// priest level:      1     2     3     4     5     6     7     8     9  10-11 12-13   14+
	{ 1, { { "10", "7", "4", "T", "T", "D", "D", "D", "D", "D", "D", "D" } } },
	{ 2, { { "16", "13", "10", "7", "4", "T", "T", "D", "D", "D", "D", "D" } } },
	{ 3, { { "19", "16", "13", "10", "7", "4", "T", "T", "D", "D", "D", "D" } } },
	{ 4, { { "19", "16", "13", "10", "7", "4", "T", "T", "D", "D", "D", "D" } } },
	{ 5, { { "20", "19", "16", "13", "10", "7", "4", "T", "T", "D", "D", "D" } } },
	{ 6, { { "--", "--", "20", "19", "16", "13", "10", "7", "4", "T", "T", "D" } } },
	{ 7, { { "--", "--", "--", "20", "19", "16", "13", "10", "7", "4", "T", "T" } } },
	{ 8, { { "--", "--", "--", "--", "20", "19", "16", "13", "10", "7", "4", "T" } } },
	{ 9, { { "--", "--", "--", "--", "--", "20", "19", "16", "13", "10", "7", "4" } } },
	{ 10, { { "--", "--", "--", "--", "--", "--", "20", "19", "16", "13", "10", "7" } } },
	{ 11, { { "--", "--", "--", "--", "--", "--", "--", "20", "19", "16", "13", "10" } } },
} };

// Reads one printed cell - a dash, a letter or a d20 target - into what the
// lookup owes for it.
TurningResult expected_from_printed(std::string_view printed)
{
	if (printed == "--")
	{
		return TurningResult{ TurningOutcome::BEYOND_POWER, 0 };
	}
	if (printed == "T")
	{
		return TurningResult{ TurningOutcome::ALWAYS_TURNED, 0 };
	}
	if (printed == "D")
	{
		return TurningResult{ TurningOutcome::DESTROYED, 0 };
	}
	return TurningResult{ TurningOutcome::ROLL_REQUIRED, std::stoi(std::string(printed)) };
}
} // namespace

// Every cell of the page, against every priest level the cell's column covers.
// The worked example below checks one column; this checks all twelve, and it is
// the only test that can see a column heading being read as a single level.
TEST(TurningTableTest, EveryCellMatchesThePrintedTable)
{
	for (const PrintedRow& row : PRINTED_TABLE)
	{
		for (size_t column = 0; column < PRINTED_COLUMNS; ++column)
		{
			const ColumnSpan span = COLUMN_SPANS[column];
			const std::string_view printed = row.cells[column];
			const TurningResult expected = expected_from_printed(printed);

			for (int level = span.firstLevel; level <= span.lastLevel; ++level)
			{
				const TurningResult actual = look_up_turning(row.hitDice, level);

				EXPECT_EQ(actual.outcome, expected.outcome)
					<< row.hitDice << " HD against a level " << level << " priest: the book prints " << printed;
				EXPECT_EQ(actual.rollNeeded, expected.rollNeeded)
					<< row.hitDice << " HD against a level " << level << " priest: the book prints " << printed;
			}
		}
	}
}

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
