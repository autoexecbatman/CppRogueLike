// file: MonsterStrengthTableTest.cpp
// Table 6, Monster Strength Scores, read as the Dungeon Master Guide prints it (PDF
// page 4857 of the local archive, counting from 1 as a PDF viewer does). Each
// expectation below is a row of the printed table, in its column order Tiny, Small,
// Man-sized, Large, Huge, Gargantuan, so a value can be checked against the page rather
// than against the code.
//
// 18/51 is a score of 18 and an exceptional percentile of 51, which is where its band
// starts; 18/00 is the percentile 100.
//
// The book's own worked example, on the page after, says to roll Strength "on Table 5"
// and Intelligence "on Table 8"; the tables it prints are 6 and 9, and those are the
// numbers cited here.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterStrengthTableTest.*

#include <gtest/gtest.h>

#include <array>

#include "src/CreatureSize.h"
#include "src/MonsterAbilityTables.h"

namespace
{
void expect_row(int roll, std::array<MonsterStrength, 6> printed)
{
	for (std::size_t column = 0; column < ALL_CREATURE_SIZE.size(); ++column)
	{
		const auto read = monster_strength_score(roll, ALL_CREATURE_SIZE.at(column));
		EXPECT_EQ(read.score, printed.at(column).score) << "roll " << roll << ", " << encode_creature_size(ALL_CREATURE_SIZE.at(column));
		EXPECT_EQ(read.exceptional, printed.at(column).exceptional) << "roll " << roll << ", " << encode_creature_size(ALL_CREATURE_SIZE.at(column));
	}
}
} // namespace

// Every row the book prints, 3 to 18, with 9 to 12 as one row.
TEST(MonsterStrengthTableTest, EveryPrintedRowIsTheBooks)
{
	expect_row(3, { { { 2, 0 }, { 4, 0 }, { 6, 0 }, { 13, 0 }, { 15, 0 }, { 18, 0 } } });
	expect_row(4, { { { 3, 0 }, { 5, 0 }, { 7, 0 }, { 14, 0 }, { 17, 0 }, { 18, 1 } } });
	expect_row(5, { { { 4, 0 }, { 6, 0 }, { 8, 0 }, { 15, 0 }, { 18, 0 }, { 18, 51 } } });
	expect_row(6, { { { 5, 0 }, { 7, 0 }, { 9, 0 }, { 16, 0 }, { 18, 1 }, { 18, 76 } } });
	expect_row(7, { { { 6, 0 }, { 8, 0 }, { 10, 0 }, { 17, 0 }, { 18, 51 }, { 18, 91 } } });
	expect_row(8, { { { 7, 0 }, { 9, 0 }, { 11, 0 }, { 18, 0 }, { 18, 76 }, { 18, 100 } } });
	for (int roll = 9; roll <= 12; ++roll)
	{
		expect_row(roll, { { { 8, 0 }, { 10, 0 }, { 12, 0 }, { 18, 1 }, { 18, 91 }, { 19, 0 } } });
	}
	expect_row(13, { { { 10, 0 }, { 11, 0 }, { 13, 0 }, { 18, 51 }, { 18, 100 }, { 20, 0 } } });
	expect_row(14, { { { 11, 0 }, { 12, 0 }, { 14, 0 }, { 18, 76 }, { 19, 0 }, { 21, 0 } } });
	expect_row(15, { { { 12, 0 }, { 13, 0 }, { 15, 0 }, { 18, 91 }, { 20, 0 }, { 22, 0 } } });
	expect_row(16, { { { 13, 0 }, { 14, 0 }, { 16, 0 }, { 18, 100 }, { 21, 0 }, { 23, 0 } } });
	expect_row(17, { { { 14, 0 }, { 15, 0 }, { 17, 0 }, { 19, 0 }, { 22, 0 }, { 24, 0 } } });
	expect_row(18, { { { 15, 0 }, { 16, 0 }, { 18, 0 }, { 20, 0 }, { 23, 0 }, { 25, 0 } } });
}

// end of file: MonsterStrengthTableTest.cpp
