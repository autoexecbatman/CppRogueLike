#pragma once

#include <array>
#include <cassert>
#include <cstddef>

#include "CreatureSize.h"
#include "DiceExpr.h"

// file: MonsterAbilityTables.h
//
// The Dungeon Master Guide's tables for a monster's ability scores, from Ability Scores
// for Monsters (PDF pages 4857-4860 of the local archive). A score is 3d6 read from the
// column for the creature's size, so a monster's Constitution is derived from its size
// and a roll rather than stored. The book offers the method for any creature "in a
// pinch", and this game takes it for every monster.
//
// Usage:
//
//   const int roll = roll_dice(dice, MONSTER_ABILITY_DICE); // the 3d6 the table is read by
//   monster_constitution_score(roll, CreatureSize::LARGE); // a 15 reads 18 for a large creature

// The 3d6 every table is read by.
inline constexpr DiceExpr MONSTER_ABILITY_DICE{ 3, 6, 0 };

// The lowest and highest those dice can roll, which bound every table's rows.
inline constexpr int MONSTER_ABILITY_ROLL_MINIMUM = MONSTER_ABILITY_DICE.min_total();
inline constexpr int MONSTER_ABILITY_ROLL_MAXIMUM = MONSTER_ABILITY_DICE.max_total();

// Table 8, Monster Constitution Scores (PDF page 4858): one row per 3d6 result from 3
// to 18, one column per size from Tiny to Gargantuan. The book prints 9 to 12 as one
// row, so those four rows repeat it.
inline constexpr std::array<std::array<int, 6>, MONSTER_ABILITY_ROLL_MAXIMUM - MONSTER_ABILITY_ROLL_MINIMUM + 1> MONSTER_CONSTITUTION_TABLE{ {
	{ 3, 5, 7, 9, 11, 13 }, // 3
	{ 4, 6, 8, 10, 12, 14 }, // 4
	{ 5, 7, 9, 11, 13, 15 }, // 5
	{ 6, 8, 10, 12, 14, 16 }, // 6
	{ 7, 9, 11, 13, 15, 17 }, // 7
	{ 8, 10, 12, 14, 16, 18 }, // 8
	{ 9, 11, 13, 15, 17, 19 }, // 9
	{ 9, 11, 13, 15, 17, 19 }, // 10
	{ 9, 11, 13, 15, 17, 19 }, // 11
	{ 9, 11, 13, 15, 17, 19 }, // 12
	{ 10, 12, 14, 16, 18, 20 }, // 13
	{ 11, 13, 15, 17, 19, 21 }, // 14
	{ 12, 14, 16, 18, 20, 22 }, // 15
	{ 13, 15, 17, 19, 21, 23 }, // 16
	{ 14, 16, 18, 20, 22, 24 }, // 17
	{ 15, 17, 19, 21, 23, 25 }, // 18
} };

// The Constitution a monster of that size has for a 3d6 roll, read from Table 8. A roll
// outside 3 to 18 is a caller that did not roll 3d6, and asserts.
//
// Example:
//   monster_constitution_score(17, CreatureSize::MEDIUM); // -> 18
//   monster_constitution_score(3, CreatureSize::TINY); // -> 3, the table's lowest
//   monster_constitution_score(18, CreatureSize::GARGANTUAN); // -> 25, its highest
[[nodiscard]] inline int monster_constitution_score(int roll, CreatureSize size)
{
	assert(roll >= MONSTER_ABILITY_ROLL_MINIMUM && roll <= MONSTER_ABILITY_ROLL_MAXIMUM && "monster_constitution_score: the roll is not a 3d6");
	const auto row = static_cast<std::size_t>(roll - MONSTER_ABILITY_ROLL_MINIMUM);
	return MONSTER_CONSTITUTION_TABLE.at(row).at(static_cast<std::size_t>(size));
}

// end of file: MonsterAbilityTables.h
