#pragma once

// file: Random/DiceExpr.h
//
// A dice expression as the game's data writes it: a count of dice, their sides
// and a flat bonus, "3d12+5". This is the fact a damage roll is made of, and
// the reason it is kept rather than a minimum and maximum is that some rules
// act on each die - fire resistance takes two off every die and lets none fall
// below one - which no range can express.
//
// Usage:
//
//   const DiceExpr claws = parse_dice_expression("2d4+1");   // -> { 2, 4, 1 }
//   claws.min_total();                                        // -> 3
//   claws.max_total();                                        // -> 9
//   const std::vector<int> rolled = roll_each_die(&dice, claws);  // two values in 1..4
//   roll_dice(&dice, claws);                                  // -> their sum plus 1
//
// parse_dice_expression reads <count>d<sides> with an optional signed bonus, or a
// bare integer, which is a fixed value with no dice - the book's "10 hit points
// per round" inside a great fire is one, and nothing per-die can touch it. It
// refuses anything else, and a count or sides of zero.

#include <cassert>
#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "RandomDice.h"

struct DiceExpr
{
	int num{ 0 };
	int sides{ 0 };
	int bonus{ 0 };

	// The least and the most the expression can roll.
	[[nodiscard]] constexpr int min_total() const noexcept { return num + bonus; }
	[[nodiscard]] constexpr int max_total() const noexcept { return num * sides + bonus; }
};

// Reads "<count>d<sides>", optionally followed by "+<bonus>" or "-<bonus>", or
// a bare integer as a fixed value.
//
// Example:
//   parse_dice_expression("1d8");      // -> { 1, 8, 0 }
//   parse_dice_expression("1d12+5");   // -> { 1, 12, 5 }
//   parse_dice_expression("5");        // -> { 0, 0, 5 }, no dice
//   parse_dice_expression("d8");       // throws std::invalid_argument
inline DiceExpr parse_dice_expression(std::string_view text)
{
	const auto fail = [text]()
	{
		throw std::invalid_argument("not a dice expression: " + std::string(text));
	};

	DiceExpr expr{};
	const char* cursor = text.data();
	const char* const end = text.data() + text.size();

	// The count, then the letter d - or the whole text is one fixed value.
	auto result = std::from_chars(cursor, end, expr.num);
	if (result.ec == std::errc{} && result.ptr == end)
	{
		if (expr.num < 0)
		{
			fail();
		}
		return DiceExpr{ 0, 0, expr.num };
	}
	if (result.ec != std::errc{} || result.ptr == end || *result.ptr != 'd' || expr.num <= 0)
	{
		fail();
	}
	cursor = result.ptr + 1;

	// The sides.
	result = std::from_chars(cursor, end, expr.sides);
	if (result.ec != std::errc{} || expr.sides <= 0)
	{
		fail();
	}
	cursor = result.ptr;

	// An optional signed bonus, and nothing after it. from_chars reads a minus
	// sign itself and refuses a plus, so the plus is stepped over here.
	if (cursor != end)
	{
		if (*cursor != '+' && *cursor != '-')
		{
			fail();
		}
		if (*cursor == '+')
		{
			++cursor;
			if (cursor == end)
			{
				fail();
			}
		}
		result = std::from_chars(cursor, end, expr.bonus);
		if (result.ec != std::errc{} || result.ptr != end)
		{
			fail();
		}
	}
	return expr;
}

// The expression as the data writes it, and the inverse of
// parse_dice_expression: parsing what this returns gives the same dice back.
// A fixed value, having no dice, is written as its number alone.
//
// Example:
//   to_text(DiceExpr{ 1, 12, 5 });   // -> "1d12+5"
//   to_text(DiceExpr{ 2, 6, -1 });   // -> "2d6-1"
//   to_text(DiceExpr{ 0, 0, 5 });    // -> "5"
inline std::string to_text(const DiceExpr& expr)
{
	if (expr.num == 0)
	{
		return std::to_string(expr.bonus);
	}
	std::string text = std::to_string(expr.num) + "d" + std::to_string(expr.sides);
	if (expr.bonus > 0)
	{
		text += "+" + std::to_string(expr.bonus);
	}
	else if (expr.bonus < 0)
	{
		// to_string writes the minus sign itself.
		text += std::to_string(expr.bonus);
	}
	return text;
}

// Rolls every die of the expression and returns each value; the bonus is the
// caller's to add, because rules that act per die must see the dice alone.
//
// Example, dice forced to 3 and 4:
//   roll_each_die(&dice, { 2, 6, 1 });   // -> { 3, 4 }
inline std::vector<int> roll_each_die(RandomDice* dice, const DiceExpr& expr)
{
	assert(dice && "roll_each_die called without dice");
	std::vector<int> rolled;
	rolled.reserve(static_cast<size_t>(expr.num));
	for (int die = 0; die < expr.num; ++die)
	{
		rolled.push_back(dice->roll(1, expr.sides));
	}
	return rolled;
}

// The whole roll: every die plus the bonus. A fixed value, having no dice, is
// its bonus.
//
// Example, dice forced to 3 and 4:
//   roll_dice(&dice, { 2, 6, 1 });   // -> 8
//   roll_dice(&dice, { 0, 0, 5 });   // -> 5
inline int roll_dice(RandomDice* dice, const DiceExpr& expr)
{
	int total = expr.bonus;
	for (const int die : roll_each_die(dice, expr))
	{
		total += die;
	}
	return total;
}
