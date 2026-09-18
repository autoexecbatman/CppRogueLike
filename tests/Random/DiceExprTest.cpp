// file: DiceExprTest.cpp
// The dice expression is the fact a damage roll is made of, read from the
// strings the data already writes: "<count>d<sides>" with an optional signed
// bonus. It accepts exactly that shape and refuses everything else, because a
// string that parses as nothing must fail where it is written, not roll as 0.
//
// The defect this pins: std::from_chars does not read a leading plus sign, so
// "1d4+1" threw inside a static initialiser and the test executable died
// before main with exit code 3 and no output.

#include <gtest/gtest.h>

#include <stdexcept>

#include "src/DiceExpr.h"
#include "src/RandomDice.h"

TEST(DiceExprTest, ReadsCountSidesAndAPositiveBonus)
{
	const DiceExpr expr = parse_dice_expression("1d4+1");

	EXPECT_EQ(expr.num, 1);
	EXPECT_EQ(expr.sides, 4);
	EXPECT_EQ(expr.bonus, 1);
	EXPECT_EQ(expr.min_total(), 2);
	EXPECT_EQ(expr.max_total(), 5);
}

TEST(DiceExprTest, ReadsANegativeBonus)
{
	const DiceExpr expr = parse_dice_expression("2d6-1");

	EXPECT_EQ(expr.num, 2);
	EXPECT_EQ(expr.sides, 6);
	EXPECT_EQ(expr.bonus, -1);
	EXPECT_EQ(expr.min_total(), 1);
	EXPECT_EQ(expr.max_total(), 11);
}

TEST(DiceExprTest, ReadsPlainDice)
{
	const DiceExpr expr = parse_dice_expression("3d12");

	EXPECT_EQ(expr.num, 3);
	EXPECT_EQ(expr.sides, 12);
	EXPECT_EQ(expr.bonus, 0);
}

// A bare integer is a fixed value with no dice: the book's "10 hit points per
// round" is one, and per-die reduction has nothing to act on.
TEST(DiceExprTest, ReadsABareIntegerAsAFixedValue)
{
	const DiceExpr expr = parse_dice_expression("5");

	EXPECT_EQ(expr.num, 0);
	EXPECT_EQ(expr.bonus, 5);
	EXPECT_EQ(expr.min_total(), 5);
	EXPECT_EQ(expr.max_total(), 5);
	RandomDice dice{};
	EXPECT_EQ(roll_dice(&dice, expr), 5);
	EXPECT_TRUE(roll_each_die(&dice, expr).empty());
}

TEST(DiceExprTest, RefusesWhatIsNotDice)
{
	EXPECT_THROW(parse_dice_expression(""), std::invalid_argument);
	EXPECT_THROW(parse_dice_expression("-5"), std::invalid_argument);
	EXPECT_THROW(parse_dice_expression("d8"), std::invalid_argument);
	EXPECT_THROW(parse_dice_expression("1d"), std::invalid_argument);
	EXPECT_THROW(parse_dice_expression("0d6"), std::invalid_argument);
	EXPECT_THROW(parse_dice_expression("1d0"), std::invalid_argument);
	EXPECT_THROW(parse_dice_expression("1d6+"), std::invalid_argument);
	EXPECT_THROW(parse_dice_expression("1d6x"), std::invalid_argument);
	EXPECT_THROW(parse_dice_expression("2-8"), std::invalid_argument);
}

// Each die is rolled on its own and the bonus is left to the caller.
TEST(DiceExprTest, RollsEachDieAndLeavesTheBonusToTheCaller)
{
	RandomDice dice{};
	dice.set_test_mode(true);
	dice.set_next_roll(3);
	dice.set_next_roll(4);

	const std::vector<int> rolled = roll_each_die(&dice, DiceExpr{ 2, 6, 1 });

	EXPECT_EQ(rolled, (std::vector<int>{ 3, 4 }));
}

TEST(DiceExprTest, RollDiceSumsAndAddsTheBonus)
{
	RandomDice dice{};
	dice.set_test_mode(true);
	dice.set_next_roll(3);
	dice.set_next_roll(4);

	EXPECT_EQ(roll_dice(&dice, DiceExpr{ 2, 6, 1 }), 8);
}

// Writing an expression out is the inverse of reading one in: the editor stores
// dice and the data file stores text, and a round trip through both must not
// change the dice.
TEST(DiceExprTest, WritesTheExpressionTheDataUses)
{
	EXPECT_EQ(to_text(DiceExpr{ 1, 8, 0 }), "1d8");
	EXPECT_EQ(to_text(DiceExpr{ 1, 12, 5 }), "1d12+5");
	EXPECT_EQ(to_text(DiceExpr{ 2, 6, -1 }), "2d6-1");
	EXPECT_EQ(to_text(DiceExpr{ 0, 0, 5 }), "5") << "a fixed value has no dice to write";
}

TEST(DiceExprTest, TextSurvivesARoundTrip)
{
	for (const DiceExpr& expr : { DiceExpr{ 1, 8, 0 }, DiceExpr{ 3, 12, 0 }, DiceExpr{ 1, 12, 5 },
		DiceExpr{ 2, 6, -1 }, DiceExpr{ 0, 0, 5 }, DiceExpr{ 10, 6, 0 } })
	{
		const DiceExpr read = parse_dice_expression(to_text(expr));
		EXPECT_EQ(read.num, expr.num) << to_text(expr);
		EXPECT_EQ(read.sides, expr.sides) << to_text(expr);
		EXPECT_EQ(read.bonus, expr.bonus) << to_text(expr);
	}
}
