#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "DiceExpr.h"

// file: MonsterHitDice.h
//
// A monster's hit dice as a Monstrous Manual entry prints them: the die and a bonus added
// once to the total, and the counts it may have, each with the experience it is worth.
// Most monsters have one count. The killer mimic's "9-10" is two, worth 2,000 and 3,000.
// The book's below-one dice are count 1 with a negative bonus, so the hairy spider's 1-1
// is { 8, -1, { { 1, 65 } } } and the huge spider's 2+2 is { 8, 2, { { 2, 270 } } }:
// 1d8-1 and 2d8+2.
//
// Usage:
//
//   const MonsterHitDice mimic{ 8, 0, { { 9, 2000 }, { 10, 3000 } } };
//   is_valid_monster_hit_dice(mimic); // -> true
//   dice_for(mimic, 10); // -> DiceExpr{ 10, 8, 0 }
//   highest_count(mimic).xp; // -> 3000, what an encounter budget pays for it

// The lowest count any monster has, since the count is its level, and the highest the
// editor offers.
inline constexpr int MONSTER_HIT_DICE_LOWEST_COUNT = 1;
inline constexpr int MONSTER_HIT_DICE_HIGHEST_COUNT = 99;

// One hit dice count a monster may have, and the experience a kill at that count is worth.
struct HitDiceCount
{
	int count{ 1 };
	int xp{ 0 };
};

// The die, the bonus added once to the total, and the counts in ascending order with no
// gaps. Hit dice nobody filled hold no count, which the loader and the creator refuse.
struct MonsterHitDice
{
	int sides{ 8 };
	int bonus{ 0 };
	std::vector<HitDiceCount> counts{};
};

// Whether the book can write these hit dice: at least one count, the first at least 1 since
// the count is the creature's level, each count one more than the last, no negative
// experience, and a die of at least one side.
//
// Example:
//   is_valid_monster_hit_dice({ 8, 0, { { 9, 2000 }, { 10, 3000 } } }); // -> true
//   is_valid_monster_hit_dice({ 8, 0, { { 9, 2000 }, { 11, 3000 } } }); // -> false, a gap
//   is_valid_monster_hit_dice({ 8, 0, {} }); // -> false, no count
[[nodiscard]] inline bool is_valid_monster_hit_dice(const MonsterHitDice& hitDice)
{
	if (hitDice.counts.empty() || hitDice.sides < 1 || hitDice.counts.front().count < MONSTER_HIT_DICE_LOWEST_COUNT)
	{
		return false;
	}
	for (size_t index = 0; index < hitDice.counts.size(); ++index)
	{
		// Each count is worth something or nothing, and follows the one before by exactly one.
		const HitDiceCount& entry = hitDice.counts.at(index);
		const bool followsByOne = index == 0 || entry.count == hitDice.counts.at(index - 1).count + 1;
		if (entry.xp < 0 || !followsByOne)
		{
			return false;
		}
	}
	return true;
}

// The dice a monster with these hit dice rolls at one of its counts.
//
// Example:
//   dice_for(MonsterHitDice{ 8, 2, { { 2, 270 } } }, 2); // -> DiceExpr{ 2, 8, 2 }, 2d8+2
[[nodiscard]] inline DiceExpr dice_for(const MonsterHitDice& hitDice, int count)
{
	return DiceExpr{ count, hitDice.sides, hitDice.bonus };
}

// The highest count and its experience. Valid hit dice always have one.
//
// Example:
//   highest_count(MonsterHitDice{ 8, 0, { { 9, 2000 }, { 10, 3000 } } }); // -> { 10, 3000 }
[[nodiscard]] inline const HitDiceCount& highest_count(const MonsterHitDice& hitDice)
{
	assert(!hitDice.counts.empty() && "highest_count: hit dice with no count");
	return hitDice.counts.back();
}

// The entry for one of the counts these hit dice hold. A count they do not hold is a caller
// that rolled outside the range, and asserts.
//
// Example:
//   count_entry(MonsterHitDice{ 8, 0, { { 9, 2000 }, { 10, 3000 } } }, 9).xp; // -> 2000
[[nodiscard]] inline const HitDiceCount& count_entry(const MonsterHitDice& hitDice, int count)
{
	const auto isThatCount = [count](const HitDiceCount& entry)
	{
		return entry.count == count;
	};
	const auto found = std::ranges::find_if(hitDice.counts, isThatCount);
	assert(found != hitDice.counts.end() && "count_entry: a count these hit dice do not hold");
	return *found;
}

// These hit dice with their lowest count moved to the given one, clamped from 1 up to the
// highest. Counts added at the bottom are worth what the old lowest was; counts dropped take
// their experience with them.
//
// Example:
//   with_lowest_count(MonsterHitDice{ 8, 0, { { 9, 2000 }, { 10, 3000 } } }, 8); // -> { { 8, 2000 }, { 9, 2000 }, { 10, 3000 } }
//   with_lowest_count(MonsterHitDice{ 8, 0, { { 9, 2000 }, { 10, 3000 } } }, 12); // -> { { 10, 3000 } }, clamped to the highest
[[nodiscard]] inline MonsterHitDice with_lowest_count(MonsterHitDice hitDice, int lowest)
{
	const int target = std::clamp(lowest, MONSTER_HIT_DICE_LOWEST_COUNT, highest_count(hitDice).count);
	// Drop counts below the new lowest, then add any the range now reaches down to.
	while (hitDice.counts.front().count < target)
	{
		hitDice.counts.erase(hitDice.counts.begin());
	}
	while (hitDice.counts.front().count > target)
	{
		const HitDiceCount below{ hitDice.counts.front().count - 1, hitDice.counts.front().xp };
		hitDice.counts.insert(hitDice.counts.begin(), below);
	}
	return hitDice;
}

// These hit dice with their highest count moved to the given one, clamped from the lowest up
// to 99. Counts added at the top are worth what the old highest was.
//
// Example:
//   with_highest_count(MonsterHitDice{ 8, 0, { { 9, 2000 } } }, 10); // -> { { 9, 2000 }, { 10, 2000 } }
//   with_highest_count(MonsterHitDice{ 8, 0, { { 9, 2000 }, { 10, 3000 } } }, 1); // -> { { 9, 2000 } }, clamped to the lowest
[[nodiscard]] inline MonsterHitDice with_highest_count(MonsterHitDice hitDice, int highest)
{
	const int target = std::clamp(highest, hitDice.counts.front().count, MONSTER_HIT_DICE_HIGHEST_COUNT);
	// Drop counts above the new highest, then add any the range now reaches up to.
	while (highest_count(hitDice).count > target)
	{
		hitDice.counts.pop_back();
	}
	while (highest_count(hitDice).count < target)
	{
		const HitDiceCount above{ highest_count(hitDice).count + 1, highest_count(hitDice).xp };
		hitDice.counts.push_back(above);
	}
	return hitDice;
}

// end of file: MonsterHitDice.h
