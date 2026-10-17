// file: MonsterCharismaTableTest.cpp
// Table 11, Monster Charisma Scores, read as the Dungeon Master Guide prints it (PDF
// page 4859 of the local archive, counting from 1 as a PDF viewer does). Each
// expectation below is a row of the printed table, in its column order Animal, Semi-,
// Low, Average, Very, High, Exceptional, Genius, Supra-genius, Godlike, so a value can
// be checked against the page rather than against the code.
//
// The table has no column for a Non- (0) creature. By the driver's ruling of
// 2026-10-17 such a creature's Intelligence is 0, as its Monstrous Manual entry prints,
// and its Wisdom and Charisma are read from the Animal column, the lowest the book prints.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterCharismaTableTest.*

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
		EXPECT_EQ(monster_charisma_score(roll, rating), printed.at(column)) << "roll " << roll << ", " << encode_intelligence_rating(rating);
	}
}
} // namespace

// Every row the book prints, 3 to 18, with 9 to 12 as one row.
TEST(MonsterCharismaTableTest, EveryPrintedRowIsTheBooks)
{
	expect_row(3, { { 1, 1, 3, 6, 8, 11, 13, 15, 17, 19 } });
	expect_row(4, { { 1, 1, 3, 6, 8, 11, 13, 15, 17, 19 } });
	expect_row(5, { { 1, 1, 4, 7, 9, 12, 14, 16, 18, 20 } });
	expect_row(6, { { 1, 1, 4, 7, 9, 12, 14, 16, 18, 20 } });
	expect_row(7, { { 1, 2, 5, 8, 10, 13, 15, 17, 19, 21 } });
	expect_row(8, { { 1, 2, 5, 8, 10, 13, 15, 17, 19, 21 } });
	for (int roll = 9; roll <= 12; ++roll)
	{
		expect_row(roll, { { 1, 3, 6, 9, 11, 14, 16, 18, 20, 22 } });
	}
	expect_row(13, { { 1, 3, 6, 9, 11, 14, 16, 18, 20, 22 } });
	expect_row(14, { { 1, 4, 7, 10, 12, 15, 17, 19, 21, 23 } });
	expect_row(15, { { 2, 4, 7, 10, 12, 15, 17, 19, 21, 23 } });
	expect_row(16, { { 2, 5, 8, 11, 13, 16, 18, 20, 22, 24 } });
	expect_row(17, { { 3, 5, 8, 11, 13, 16, 18, 20, 22, 24 } });
	expect_row(18, { { 3, 6, 9, 12, 14, 17, 19, 21, 23, 25 } });
}

// A Non- (0) creature has no column; it reads the Animal column's Charisma.
TEST(MonsterCharismaTableTest, ANonIntelligentCreatureReadsTheAnimalColumn)
{
	EXPECT_EQ(monster_charisma_score(3, IntelligenceRating::NON), 1);
	EXPECT_EQ(monster_charisma_score(4, IntelligenceRating::NON), 1);
	EXPECT_EQ(monster_charisma_score(5, IntelligenceRating::NON), 1);
	EXPECT_EQ(monster_charisma_score(6, IntelligenceRating::NON), 1);
	EXPECT_EQ(monster_charisma_score(7, IntelligenceRating::NON), 1);
	EXPECT_EQ(monster_charisma_score(8, IntelligenceRating::NON), 1);
	EXPECT_EQ(monster_charisma_score(9, IntelligenceRating::NON), 1);
	EXPECT_EQ(monster_charisma_score(10, IntelligenceRating::NON), 1);
	EXPECT_EQ(monster_charisma_score(11, IntelligenceRating::NON), 1);
	EXPECT_EQ(monster_charisma_score(12, IntelligenceRating::NON), 1);
	EXPECT_EQ(monster_charisma_score(13, IntelligenceRating::NON), 1);
	EXPECT_EQ(monster_charisma_score(14, IntelligenceRating::NON), 1);
	EXPECT_EQ(monster_charisma_score(15, IntelligenceRating::NON), 2);
	EXPECT_EQ(monster_charisma_score(16, IntelligenceRating::NON), 2);
	EXPECT_EQ(monster_charisma_score(17, IntelligenceRating::NON), 3);
	EXPECT_EQ(monster_charisma_score(18, IntelligenceRating::NON), 3);
}

// end of file: MonsterCharismaTableTest.cpp
