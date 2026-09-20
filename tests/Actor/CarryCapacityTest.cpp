// file: CarryCapacityTest.cpp
// What a creature can carry, in pounds, against the printed tables. The game used to
// compute it as 50 + (Strength - 10) * 5, which is no column of any table: it gave a
// Strength 19 hill giant 95 pounds where the book allows hundreds.
//
// Expected values are Table 47, Character Encumbrance (PDF page 159), whose Max. Carried
// Weight column is "the most weight (in pounds) your character can carry and still move".
// Table 47 stops at 18/00, so Strength 1 and 19-25 - which a girdle of giant strength
// reaches - take Table 1's Max. Press (PDF pages 30-31) instead. The two columns agree on
// 14 of the 16 rows both print; they differ at Strength 2 (6 against 5) and at 10-11 (110
// against 115). Where Table 47 prints a number it wins, because it is the table about
// carrying rather than about lifting overhead.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=CarryCapacityTest.*

#include <gtest/gtest.h>

#include "src/Creature.h"
#include "src/InventoryOperations.h"
#include "tests/mocks/MockGameContext.h"

class CarryCapacityTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
	}

	// The capacity of a carrier at this Strength, with no exceptional percentile.
	int capacity_at(int strength)
	{
		carrier.set_strength(strength);
		carrier.set_exceptional_strength(0);
		return InventoryOperations::get_max_weight(carrier, *ctx.dataManager);
	}

	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
	Creature carrier{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "carrier", 0 } };
};

TEST_F(CarryCapacityTest, AnAverageStrengthCarriesTheTablesHundredAndTen)
{
	// Table 47, row 10-11: Max. Carried Weight 110.
	EXPECT_EQ(capacity_at(10), 110);
	EXPECT_EQ(capacity_at(11), 110);
}

TEST_F(CarryCapacityTest, EachPrintedRowCarriesItsOwnNumber)
{
	// Table 47, rows 3 through 18: the Max. Carried Weight column, read down.
	EXPECT_EQ(capacity_at(3), 10);
	EXPECT_EQ(capacity_at(5), 25);
	EXPECT_EQ(capacity_at(9), 90);
	EXPECT_EQ(capacity_at(14), 170);
	EXPECT_EQ(capacity_at(16), 195);
	EXPECT_EQ(capacity_at(18), 255);
}

TEST_F(CarryCapacityTest, ExceptionalStrengthIsAnsweredFromItsOwnBand)
{
	carrier.set_strength(18);

	// Table 47's four exceptional rows: 18/01-50 is 280, and 18/00 is 480.
	carrier.set_exceptional_strength(50);
	EXPECT_EQ(InventoryOperations::get_max_weight(carrier, *ctx.dataManager), 280);

	carrier.set_exceptional_strength(100);
	EXPECT_EQ(InventoryOperations::get_max_weight(carrier, *ctx.dataManager), 480);
}

TEST_F(CarryCapacityTest, GiantStrengthCarriesWhatTableOnePrints)
{
	// Above 18/00 Table 47 stops, so these are Table 1's Max. Press: a hill giant's 19
	// and a titan's 25.
	EXPECT_EQ(capacity_at(19), 640);
	EXPECT_EQ(capacity_at(25), 1750);
}
