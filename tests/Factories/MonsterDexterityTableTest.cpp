// file: MonsterDexterityTableTest.cpp
// Table 7, Monster Dexterity Scores, read as the Dungeon Master Guide prints it (PDF
// page 4857 of the local archive, counting from 1 as a PDF viewer does). Each
// expectation below is a row of the printed table, in its column order Tiny, Small,
// Man-sized, Large, Huge, Gargantuan, so a value can be checked against the page rather
// than against the code.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterDexterityTableTest.*

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
		const auto read =
			monster_dexterity_score(roll, ALL_CREATURE_SIZE.at(column));
		EXPECT_EQ(read, printed.at(column))
			<< "roll " << roll << ", "
			<< encode_creature_size(ALL_CREATURE_SIZE.at(column));
	}
}
} // namespace

// Every row the book prints, 3 to 18, with 9 to 12 as one row.
TEST(MonsterDexterityTableTest, EveryPrintedRowIsTheBooks)
{
	expect_row(3, { { 13, 11, 9, 7, 5, 3 } });
	expect_row(4, { { 14, 12, 10, 8, 6, 4 } });
	expect_row(5, { { 15, 13, 11, 9, 7, 5 } });
	expect_row(6, { { 16, 14, 12, 10, 8, 6 } });
	expect_row(7, { { 17, 15, 13, 11, 9, 7 } });
	expect_row(8, { { 18, 16, 14, 12, 10, 8 } });
	for (int roll = 9; roll <= 12; ++roll)
	{
		expect_row(roll, { { 19, 17, 15, 13, 11, 9 } });
	}
	expect_row(13, { { 20, 18, 16, 14, 12, 10 } });
	expect_row(14, { { 21, 19, 17, 15, 13, 11 } });
	expect_row(15, { { 22, 20, 18, 16, 14, 12 } });
	expect_row(16, { { 23, 21, 19, 17, 15, 13 } });
	expect_row(17, { { 24, 22, 20, 18, 16, 14 } });
	expect_row(18, { { 25, 23, 21, 19, 17, 15 } });
}

// end of file: MonsterDexterityTableTest.cpp
