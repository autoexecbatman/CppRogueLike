// file: MonsterIntelligenceTableTest.cpp
// Table 9, Monster Intelligence Scores, read as the Dungeon Master Guide prints it (PDF
// page 4858 of the local archive, counting from 1 as a PDF viewer does). Each
// expectation below is a row of the printed table, in its column order Animal, Semi-,
// Low, Average, Very, High, Exceptional, Genius, Supra-genius, Godlike, so a value can
// be checked against the page rather than against the code.
//
// The book's own worked example, on the page after, says to roll Strength "on Table 5"
// and Intelligence "on Table 8"; the tables it prints are 6 and 9, and those are the
// numbers cited here.
//
// The table has no column for a Non- (0) creature. By the driver's ruling of
// 2026-10-17 such a creature's Intelligence is 0, as its Monstrous Manual entry prints,
// and its Wisdom and Charisma are read from the Animal column, the lowest the book prints.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterIntelligenceTableTest.*

#include <gtest/gtest.h>

#include <array>

#include "src/IntelligenceRating.h"
#include "src/MonsterAbilityTables.h"

namespace
{
// The ratings the table prints a column for: every one after NON.
void expect_row(int roll, std::array<int, 10> printed)
{
	for (std::size_t column = 0; column < printed.size(); ++column)
	{
		const IntelligenceRating rating = ALL_INTELLIGENCE_RATING.at(column + 1);
		EXPECT_EQ(monster_intelligence_score(roll, rating), printed.at(column)) << "roll " << roll << ", " << encode_intelligence_rating(rating);
	}
}
} // namespace

// Every row the book prints, 3 to 18, with 9 to 12 as one row.
TEST(MonsterIntelligenceTableTest, EveryPrintedRowIsTheBooks)
{
	expect_row(3, { { 1, 1, 3, 6, 7, 9, 10, 13, 15, 17 } });
	expect_row(4, { { 1, 1, 3, 7, 7, 10, 11, 14, 16, 18 } });
	expect_row(5, { { 1, 1, 4, 7, 8, 11, 13, 15, 17, 19 } });
	expect_row(6, { { 1, 1, 4, 8, 8, 11, 13, 16, 17, 19 } });
	expect_row(7, { { 1, 2, 5, 8, 9, 12, 14, 16, 18, 20 } });
	expect_row(8, { { 1, 2, 5, 9, 9, 12, 14, 17, 18, 20 } });
	for (int roll = 9; roll <= 12; ++roll)
	{
		expect_row(roll, { { 1, 3, 6, 9, 11, 13, 15, 17, 19, 21 } });
	}
	expect_row(13, { { 1, 3, 6, 10, 11, 13, 15, 18, 19, 21 } });
	expect_row(14, { { 2, 4, 7, 10, 12, 14, 16, 19, 20, 22 } });
	expect_row(15, { { 2, 4, 7, 11, 12, 15, 17, 20, 21, 22 } });
	expect_row(16, { { 3, 5, 8, 11, 13, 16, 18, 21, 22, 23 } });
	expect_row(17, { { 3, 5, 8, 12, 14, 17, 19, 22, 23, 24 } });
	expect_row(18, { { 4, 6, 9, 13, 15, 19, 20, 23, 24, 25 } });
}

// A Non- (0) creature has no column; its Intelligence is the 0 its entry prints.
TEST(MonsterIntelligenceTableTest, ANonIntelligentCreatureHasIntelligenceZero)
{
	for (int roll = 3; roll <= 18; ++roll)
	{
		EXPECT_EQ(monster_intelligence_score(roll, IntelligenceRating::NON), 0) << "roll " << roll;
	}
}

// end of file: MonsterIntelligenceTableTest.cpp
