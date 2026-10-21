// file: WordingTest.cpp
// A count beside its noun: the singular for exactly one, the plural for every other count,
// none included, and the plural taken as given rather than built; and a duration in rounds.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=WordingTest.*

#include <gtest/gtest.h>

#include "src/Wording.h"

TEST(WordingTest, OneTakesTheSingular)
{
	EXPECT_EQ(Wording::counted(1, "item", "items"), "1 item");
}

TEST(WordingTest, ManyTakeThePlural)
{
	EXPECT_EQ(Wording::counted(2, "item", "items"), "2 items");
	EXPECT_EQ(Wording::counted(12, "creature", "creatures"), "12 creatures");
}

// English counts none in the plural.
TEST(WordingTest, NoneTakesThePlural)
{
	EXPECT_EQ(Wording::counted(0, "item", "items"), "0 items");
}

// The plural is whatever the caller passes, so an irregular one comes out right.
TEST(WordingTest, AnIrregularPluralIsUsedAsGiven)
{
	EXPECT_EQ(Wording::counted(1, "foot", "feet"), "1 foot");
	EXPECT_EQ(Wording::counted(3, "foot", "feet"), "3 feet");
}

// A duration is a count of rounds, by the same rule: one round, and rounds for any other count.
TEST(WordingTest, ADurationIsWordedInRounds)
{
	EXPECT_EQ(Wording::rounds(1), "1 round");
	EXPECT_EQ(Wording::rounds(3), "3 rounds");
	EXPECT_EQ(Wording::rounds(0), "0 rounds");
}

// end of file: WordingTest.cpp
