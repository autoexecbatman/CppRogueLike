// file: MonsterConstitutionTableTest.cpp
// Table 8, Monster Constitution Scores, read as the Dungeon Master Guide prints it (PDF
// page 4858 of the local archive). Each expectation below is a row of the printed table,
// in its column order Tiny, Small, Man-sized, Large, Huge, Gargantuan, so a value can be
// checked against the page rather than against the code.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterConstitutionTableTest.*

#include <gtest/gtest.h>

#include <array>

#include "src/CreatureSize.h"
#include "src/MonsterAbilityTables.h"

namespace
{
void expect_row(int roll, std::array<int, 6> printed)
{
	for (std::size_t column = 0; column < ALL_CREATURE_SIZE.size(); ++column)
	{
		EXPECT_EQ(
			monster_constitution_score(roll, ALL_CREATURE_SIZE.at(column)),
			printed.at(column))
			<< "roll " << roll << ", "
			<< encode_creature_size(ALL_CREATURE_SIZE.at(column));
	}
}
} // namespace

TEST(MonsterConstitutionTableTest, TheLowestRowIsTheBooks)
{
	expect_row(3, { 3, 5, 7, 9, 11, 13 });
}

TEST(MonsterConstitutionTableTest, TheRowBeforeTheMiddleIsTheBooks)
{
	expect_row(8, { 8, 10, 12, 14, 16, 18 });
}

// The book prints 9 to 12 as one row, so every roll in it reads the same scores.
TEST(MonsterConstitutionTableTest, NineToTwelveAreOneRow)
{
	for (int roll = 9; roll <= 12; ++roll)
	{
		expect_row(roll, { 9, 11, 13, 15, 17, 19 });
	}
}

TEST(MonsterConstitutionTableTest, TheRowAfterTheMiddleIsTheBooks)
{
	expect_row(13, { 10, 12, 14, 16, 18, 20 });
}

TEST(MonsterConstitutionTableTest, TheFifteenRowIsTheBooks)
{
	expect_row(15, { 12, 14, 16, 18, 20, 22 });
}

TEST(MonsterConstitutionTableTest, TheHighestRowIsTheBooks)
{
	expect_row(18, { 15, 17, 19, 21, 23, 25 });
}

// end of file: MonsterConstitutionTableTest.cpp
