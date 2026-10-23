#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <optional>

#include "CreatureSize.h"
#include "DiceExpr.h"
#include "IntelligenceRating.h"
#include "MonsterStrength.h"

class Creature;
class RandomDice;

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
//   roll_monster_ability_scores(mimic, CreatureSize::LARGE, IntelligenceRating::SEMI, std::nullopt, *ctx.dice); // all six at once

// The 3d6 every table is read by.
inline constexpr DiceExpr MONSTER_ABILITY_DICE{ 3, 6, 0 };

// The lowest and highest those dice can roll, which bound every table's rows.
inline constexpr int MONSTER_ABILITY_ROLL_MINIMUM =
	MONSTER_ABILITY_DICE.min_total();
inline constexpr int MONSTER_ABILITY_ROLL_MAXIMUM =
	MONSTER_ABILITY_DICE.max_total();

// Table 8, Monster Constitution Scores (PDF page 4858): one row per 3d6 result from 3
// to 18, one column per size from Tiny to Gargantuan. The book prints 9 to 12 as one
// row, so those four rows repeat it.
inline constexpr std::array<
	std::array<int, 6>,
	MONSTER_ABILITY_ROLL_MAXIMUM - MONSTER_ABILITY_ROLL_MINIMUM + 1>
	MONSTER_CONSTITUTION_TABLE{ {
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
	assert(
		roll >= MONSTER_ABILITY_ROLL_MINIMUM &&
		roll <= MONSTER_ABILITY_ROLL_MAXIMUM &&
		"monster_constitution_score: the roll is not a 3d6");
	const auto row =
		static_cast<std::size_t>(roll - MONSTER_ABILITY_ROLL_MINIMUM);
	return MONSTER_CONSTITUTION_TABLE.at(row).at(
		static_cast<std::size_t>(size));
}

// Table 6, Monster Strength Scores (PDF page 4857): rows and columns as Table 8's. The
// book's worked example calls it Table 5; the table it prints is 6. An 18/51 cell is the
// floor of the band it names, and 18/00 is the percentile 100.
inline constexpr std::array<
	std::array<MonsterStrength, 6>,
	MONSTER_ABILITY_ROLL_MAXIMUM - MONSTER_ABILITY_ROLL_MINIMUM + 1>
	MONSTER_STRENGTH_TABLE{ {
		{
			{
				{ 2, 0 },
				{ 4, 0 },
				{ 6, 0 },
				{ 13, 0 },
				{ 15, 0 },
				{ 18, 0 },
			},
		}, // 3
		{
			{
				{ 3, 0 },
				{ 5, 0 },
				{ 7, 0 },
				{ 14, 0 },
				{ 17, 0 },
				{ 18, 1 },
			},
		}, // 4
		{
			{
				{ 4, 0 },
				{ 6, 0 },
				{ 8, 0 },
				{ 15, 0 },
				{ 18, 0 },
				{ 18, 51 },
			},
		}, // 5
		{
			{
				{ 5, 0 },
				{ 7, 0 },
				{ 9, 0 },
				{ 16, 0 },
				{ 18, 1 },
				{ 18, 76 },
			},
		}, // 6
		{
			{
				{ 6, 0 },
				{ 8, 0 },
				{ 10, 0 },
				{ 17, 0 },
				{ 18, 51 },
				{ 18, 91 },
			},
		}, // 7
		{
			{
				{ 7, 0 },
				{ 9, 0 },
				{ 11, 0 },
				{ 18, 0 },
				{ 18, 76 },
				{ 18, 100 },
			},
		}, // 8
		{
			{
				{ 8, 0 },
				{ 10, 0 },
				{ 12, 0 },
				{ 18, 1 },
				{ 18, 91 },
				{ 19, 0 },
			},
		}, // 9
		{
			{
				{ 8, 0 },
				{ 10, 0 },
				{ 12, 0 },
				{ 18, 1 },
				{ 18, 91 },
				{ 19, 0 },
			},
		}, // 10
		{
			{
				{ 8, 0 },
				{ 10, 0 },
				{ 12, 0 },
				{ 18, 1 },
				{ 18, 91 },
				{ 19, 0 },
			},
		}, // 11
		{
			{
				{ 8, 0 },
				{ 10, 0 },
				{ 12, 0 },
				{ 18, 1 },
				{ 18, 91 },
				{ 19, 0 },
			},
		}, // 12
		{
			{
				{ 10, 0 },
				{ 11, 0 },
				{ 13, 0 },
				{ 18, 51 },
				{ 18, 100 },
				{ 20, 0 },
			},
		}, // 13
		{
			{
				{ 11, 0 },
				{ 12, 0 },
				{ 14, 0 },
				{ 18, 76 },
				{ 19, 0 },
				{ 21, 0 },
			},
		}, // 14
		{
			{
				{ 12, 0 },
				{ 13, 0 },
				{ 15, 0 },
				{ 18, 91 },
				{ 20, 0 },
				{ 22, 0 },
			},
		}, // 15
		{
			{
				{ 13, 0 },
				{ 14, 0 },
				{ 16, 0 },
				{ 18, 100 },
				{ 21, 0 },
				{ 23, 0 },
			},
		}, // 16
		{
			{
				{ 14, 0 },
				{ 15, 0 },
				{ 17, 0 },
				{ 19, 0 },
				{ 22, 0 },
				{ 24, 0 },
			},
		}, // 17
		{
			{
				{ 15, 0 },
				{ 16, 0 },
				{ 18, 0 },
				{ 20, 0 },
				{ 23, 0 },
				{ 25, 0 },
			},
		}, // 18
	} };

// Table 7, Monster Dexterity Scores (PDF pages 4857-4858): rows and columns as Table 8's.
inline constexpr std::array<
	std::array<int, 6>,
	MONSTER_ABILITY_ROLL_MAXIMUM - MONSTER_ABILITY_ROLL_MINIMUM + 1>
	MONSTER_DEXTERITY_TABLE{ {
		{ 13, 11, 9, 7, 5, 3 }, // 3
		{ 14, 12, 10, 8, 6, 4 }, // 4
		{ 15, 13, 11, 9, 7, 5 }, // 5
		{ 16, 14, 12, 10, 8, 6 }, // 6
		{ 17, 15, 13, 11, 9, 7 }, // 7
		{ 18, 16, 14, 12, 10, 8 }, // 8
		{ 19, 17, 15, 13, 11, 9 }, // 9
		{ 19, 17, 15, 13, 11, 9 }, // 10
		{ 19, 17, 15, 13, 11, 9 }, // 11
		{ 19, 17, 15, 13, 11, 9 }, // 12
		{ 20, 18, 16, 14, 12, 10 }, // 13
		{ 21, 19, 17, 15, 13, 11 }, // 14
		{ 22, 20, 18, 16, 14, 12 }, // 15
		{ 23, 21, 19, 17, 15, 13 }, // 16
		{ 24, 22, 20, 18, 16, 14 }, // 17
		{ 25, 23, 21, 19, 17, 15 }, // 18
	} };

// The columns Tables 9 to 11 print, one per rating from Animal to Godlike. NON has none.
inline constexpr std::size_t MONSTER_RATING_COLUMNS =
	ALL_INTELLIGENCE_RATING.size() - 1;

// Table 9, Monster Intelligence Scores (PDF page 4858): one row per 3d6 result, one
// column per rating from Animal to Godlike. The book's worked example calls it Table 8.
inline constexpr std::array<
	std::array<int, MONSTER_RATING_COLUMNS>,
	MONSTER_ABILITY_ROLL_MAXIMUM - MONSTER_ABILITY_ROLL_MINIMUM + 1>
	MONSTER_INTELLIGENCE_TABLE{ {
		{ 1, 1, 3, 6, 7, 9, 10, 13, 15, 17 }, // 3
		{ 1, 1, 3, 7, 7, 10, 11, 14, 16, 18 }, // 4
		{ 1, 1, 4, 7, 8, 11, 13, 15, 17, 19 }, // 5
		{ 1, 1, 4, 8, 8, 11, 13, 16, 17, 19 }, // 6
		{ 1, 2, 5, 8, 9, 12, 14, 16, 18, 20 }, // 7
		{ 1, 2, 5, 9, 9, 12, 14, 17, 18, 20 }, // 8
		{ 1, 3, 6, 9, 11, 13, 15, 17, 19, 21 }, // 9
		{ 1, 3, 6, 9, 11, 13, 15, 17, 19, 21 }, // 10
		{ 1, 3, 6, 9, 11, 13, 15, 17, 19, 21 }, // 11
		{ 1, 3, 6, 9, 11, 13, 15, 17, 19, 21 }, // 12
		{ 1, 3, 6, 10, 11, 13, 15, 18, 19, 21 }, // 13
		{ 2, 4, 7, 10, 12, 14, 16, 19, 20, 22 }, // 14
		{ 2, 4, 7, 11, 12, 15, 17, 20, 21, 22 }, // 15
		{ 3, 5, 8, 11, 13, 16, 18, 21, 22, 23 }, // 16
		{ 3, 5, 8, 12, 14, 17, 19, 22, 23, 24 }, // 17
		{ 4, 6, 9, 13, 15, 19, 20, 23, 24, 25 }, // 18
	} };

// Table 10, Monster Wisdom Scores (PDF page 4859): rows and columns as Table 9's.
inline constexpr std::array<
	std::array<int, MONSTER_RATING_COLUMNS>,
	MONSTER_ABILITY_ROLL_MAXIMUM - MONSTER_ABILITY_ROLL_MINIMUM + 1>
	MONSTER_WISDOM_TABLE{ {
		{ 1, 2, 3, 6, 8, 10, 12, 14, 16, 18 }, // 3
		{ 1, 2, 3, 6, 8, 10, 12, 14, 16, 18 }, // 4
		{ 1, 2, 4, 7, 9, 11, 13, 15, 17, 19 }, // 5
		{ 2, 3, 4, 7, 9, 11, 13, 15, 17, 19 }, // 6
		{ 2, 3, 5, 8, 10, 12, 14, 16, 18, 20 }, // 7
		{ 2, 3, 5, 8, 10, 12, 14, 16, 18, 20 }, // 8
		{ 3, 4, 6, 9, 11, 13, 15, 17, 19, 21 }, // 9
		{ 3, 4, 6, 9, 11, 13, 15, 17, 19, 21 }, // 10
		{ 3, 4, 6, 9, 11, 13, 15, 17, 19, 21 }, // 11
		{ 3, 4, 6, 9, 11, 13, 15, 17, 19, 21 }, // 12
		{ 3, 4, 6, 9, 11, 13, 15, 17, 19, 21 }, // 13
		{ 3, 4, 7, 10, 12, 14, 16, 18, 20, 22 }, // 14
		{ 4, 5, 7, 10, 12, 14, 16, 18, 20, 22 }, // 15
		{ 4, 5, 8, 11, 13, 15, 17, 19, 21, 23 }, // 16
		{ 4, 5, 8, 11, 13, 16, 18, 20, 22, 24 }, // 17
		{ 5, 6, 9, 12, 14, 17, 19, 21, 23, 25 }, // 18
	} };

// Table 11, Monster Charisma Scores (PDF page 4859): rows and columns as Table 9's.
inline constexpr std::array<
	std::array<int, MONSTER_RATING_COLUMNS>,
	MONSTER_ABILITY_ROLL_MAXIMUM - MONSTER_ABILITY_ROLL_MINIMUM + 1>
	MONSTER_CHARISMA_TABLE{ {
		{ 1, 1, 3, 6, 8, 11, 13, 15, 17, 19 }, // 3
		{ 1, 1, 3, 6, 8, 11, 13, 15, 17, 19 }, // 4
		{ 1, 1, 4, 7, 9, 12, 14, 16, 18, 20 }, // 5
		{ 1, 1, 4, 7, 9, 12, 14, 16, 18, 20 }, // 6
		{ 1, 2, 5, 8, 10, 13, 15, 17, 19, 21 }, // 7
		{ 1, 2, 5, 8, 10, 13, 15, 17, 19, 21 }, // 8
		{ 1, 3, 6, 9, 11, 14, 16, 18, 20, 22 }, // 9
		{ 1, 3, 6, 9, 11, 14, 16, 18, 20, 22 }, // 10
		{ 1, 3, 6, 9, 11, 14, 16, 18, 20, 22 }, // 11
		{ 1, 3, 6, 9, 11, 14, 16, 18, 20, 22 }, // 12
		{ 1, 3, 6, 9, 11, 14, 16, 18, 20, 22 }, // 13
		{ 1, 4, 7, 10, 12, 15, 17, 19, 21, 23 }, // 14
		{ 2, 4, 7, 10, 12, 15, 17, 19, 21, 23 }, // 15
		{ 2, 5, 8, 11, 13, 16, 18, 20, 22, 24 }, // 16
		{ 3, 5, 8, 11, 13, 16, 18, 20, 22, 24 }, // 17
		{ 3, 6, 9, 12, 14, 17, 19, 21, 23, 25 }, // 18
	} };

// The row of a 3d6 roll in any of these tables. A roll outside 3 to 18 is a caller that
// did not roll 3d6, and asserts.
//
// Example:
//   monster_ability_row(3); // -> 0
//   monster_ability_row(18); // -> 15
[[nodiscard]] inline std::size_t monster_ability_row(int roll)
{
	assert(
		roll >= MONSTER_ABILITY_ROLL_MINIMUM &&
		roll <= MONSTER_ABILITY_ROLL_MAXIMUM &&
		"monster_ability_row: the roll is not a 3d6");
	return static_cast<std::size_t>(roll - MONSTER_ABILITY_ROLL_MINIMUM);
}

// The column Tables 9 to 11 print for a rating. NON has no column; its readers decide
// what it reads, and asking for its column asserts.
//
// Example:
//   monster_rating_column(IntelligenceRating::ANIMAL); // -> 0
//   monster_rating_column(IntelligenceRating::GODLIKE); // -> 9
[[nodiscard]] inline std::size_t monster_rating_column(
	IntelligenceRating rating)
{
	assert(
		rating != IntelligenceRating::NON &&
		"monster_rating_column: Tables 9 to 11 print no column for a non-intelligent creature");
	return static_cast<std::size_t>(rating) - 1;
}

// The Strength a monster of that size has for a 3d6 roll, read from Table 6.
//
// Example:
//   monster_strength_score(10, CreatureSize::MEDIUM); // -> { 12, 0 }, the book's rust monster
//   monster_strength_score(13, CreatureSize::LARGE); // -> { 18, 51 }, 18/51
[[nodiscard]] inline MonsterStrength monster_strength_score(
	int roll,
	CreatureSize size)
{
	return MONSTER_STRENGTH_TABLE.at(monster_ability_row(roll))
		.at(static_cast<std::size_t>(size));
}

// The Dexterity a monster of that size has for a 3d6 roll, read from Table 7.
//
// Example:
//   monster_dexterity_score(3, CreatureSize::TINY); // -> 13
//   monster_dexterity_score(18, CreatureSize::GARGANTUAN); // -> 15
[[nodiscard]] inline int monster_dexterity_score(int roll, CreatureSize size)
{
	return MONSTER_DEXTERITY_TABLE.at(monster_ability_row(roll))
		.at(static_cast<std::size_t>(size));
}

// The Intelligence a monster of that rating has for a 3d6 roll, read from Table 9. A
// non-intelligent creature has the 0 its Monstrous Manual entry prints, whatever it rolls.
//
// Example:
//   monster_intelligence_score(18, IntelligenceRating::ANIMAL); // -> 4, the book's rust monster
//   monster_intelligence_score(18, IntelligenceRating::NON); // -> 0
[[nodiscard]] inline int monster_intelligence_score(
	int roll,
	IntelligenceRating rating)
{
	const std::size_t row = monster_ability_row(roll);
	if (rating == IntelligenceRating::NON)
	{
		return 0;
	}
	return MONSTER_INTELLIGENCE_TABLE.at(row).at(monster_rating_column(rating));
}

// The Wisdom a monster of that rating has for a 3d6 roll, read from Table 10. A
// non-intelligent creature reads the Animal column, the lowest the book prints.
//
// Example:
//   monster_wisdom_score(18, IntelligenceRating::GODLIKE); // -> 25
//   monster_wisdom_score(18, IntelligenceRating::NON); // -> 5, the Animal column's
[[nodiscard]] inline int monster_wisdom_score(
	int roll,
	IntelligenceRating rating)
{
	const IntelligenceRating column =
		rating == IntelligenceRating::NON ? IntelligenceRating::ANIMAL : rating;
	return MONSTER_WISDOM_TABLE.at(monster_ability_row(roll))
		.at(monster_rating_column(column));
}

// The Charisma a monster of that rating has for a 3d6 roll, read from Table 11. A
// non-intelligent creature reads the Animal column, the lowest the book prints.
//
// Example:
//   monster_charisma_score(3, IntelligenceRating::LOW); // -> 3
//   monster_charisma_score(18, IntelligenceRating::NON); // -> 3, the Animal column's
[[nodiscard]] inline int monster_charisma_score(
	int roll,
	IntelligenceRating rating)
{
	const IntelligenceRating column =
		rating == IntelligenceRating::NON ? IntelligenceRating::ANIMAL : rating;
	return MONSTER_CHARISMA_TABLE.at(monster_ability_row(roll))
		.at(monster_rating_column(column));
}

// Rolls a monster's six ability scores, one 3d6 each in the book's order, and reads them
// by its size (Tables 6 to 8) and its Intelligence rating (Tables 9 to 11). A Strength its
// Monstrous Manual entry prints is set as given and rolls no die; otherwise Table 6 is
// read, and above 18 lands as its score and exceptional percentile. Defined in
// MonsterAbilityTables.cpp.
//
// Example, every die at its highest:
//   roll_monster_ability_scores(spider, CreatureSize::TINY, IntelligenceRating::LOW, std::nullopt, dice);
//   spider.get_dexterity(); // -> 25
//   spider.get_intelligence(); // -> 9
//   roll_monster_ability_scores(ogre, CreatureSize::LARGE, IntelligenceRating::LOW, MonsterStrength{ 18, 100 }, dice);
//   ogre.get_exceptional_strength(); // -> 100, and the first three dice were Dexterity's
void roll_monster_ability_scores(
	Creature& creature,
	CreatureSize size,
	IntelligenceRating rating,
	std::optional<MonsterStrength> printedStrength,
	RandomDice& dice);

// end of file: MonsterAbilityTables.h
