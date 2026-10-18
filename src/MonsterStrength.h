#pragma once

// file: MonsterStrength.h
//
// A monster's Strength as the book writes it: a score, and at 18 the exceptional
// percentile a warrior's 18 carries. Table 6 of the Dungeon Master Guide produces one
// for any monster by its size, and a Monstrous Manual entry that prints its own figure
// is authored as one in monsters.json.
//
// Usage:
//
//   const MonsterStrength pitFiend{ 18, 100 }; // 18/00, the pit fiend's printed Strength
//   const MonsterStrength hillGiant{ 19, 0 }; // a plain 19
//   is_valid_monster_strength(pitFiend); // -> true
//   is_valid_monster_strength(MonsterStrength{ 17, 50 }); // -> false, a percentile needs an 18

// The scores strength.json has a row for, which bound every Strength a monster may hold.
inline constexpr int MONSTER_STRENGTH_SCORE_MINIMUM = 1;
inline constexpr int MONSTER_STRENGTH_SCORE_MAXIMUM = 25;

// The exceptional percentile's range: 0 is none, 1 to 99 are 18/01 to 18/99, and 100 is
// 18/00.
inline constexpr int MONSTER_STRENGTH_EXCEPTIONAL_NONE = 0;
inline constexpr int MONSTER_STRENGTH_EXCEPTIONAL_MAXIMUM = 100;

// The only score an exceptional percentile can ride on.
inline constexpr int MONSTER_STRENGTH_EXCEPTIONAL_SCORE = 18;

// A Strength score and its exceptional percentile. exceptional is 0 for no percentile,
// and 1 to 100 is the percentile, 100 being 18/00; only a score of 18 carries one, so
// 18/51 is { 18, 51 } and a plain 19 is { 19, 0 }.
struct MonsterStrength
{
	int score{ 0 };
	int exceptional{ 0 };
};

// Whether a Strength is one the book can write: a score strength.json has a row for, and
// a percentile only beside an 18.
//
// Example:
//   is_valid_monster_strength(MonsterStrength{ 18, 76 }); // -> true, 18/76
//   is_valid_monster_strength(MonsterStrength{ 22, 0 }); // -> true
//   is_valid_monster_strength(MonsterStrength{ 19, 50 }); // -> false, no percentile above 18
//   is_valid_monster_strength(MonsterStrength{ 26, 0 }); // -> false, no row
[[nodiscard]] constexpr bool is_valid_monster_strength(MonsterStrength strength) noexcept
{
	const bool hasRow = strength.score >= MONSTER_STRENGTH_SCORE_MINIMUM && strength.score <= MONSTER_STRENGTH_SCORE_MAXIMUM;
	const bool percentileInRange = strength.exceptional >= MONSTER_STRENGTH_EXCEPTIONAL_NONE && strength.exceptional <= MONSTER_STRENGTH_EXCEPTIONAL_MAXIMUM;
	const bool percentileOnAnEighteen = strength.exceptional == MONSTER_STRENGTH_EXCEPTIONAL_NONE || strength.score == MONSTER_STRENGTH_EXCEPTIONAL_SCORE;
	return hasRow && percentileInRange && percentileOnAnEighteen;
}

// end of file: MonsterStrength.h
