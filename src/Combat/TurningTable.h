#pragma once

#include <array>

// file: TurningTable.h
//
// AD&D 2e Table 61, "Turning Undead" (Player's Handbook page 208-209), and the
// lookup that reads it. The table alone: who may attempt a turn, and what a
// result does to the creature, belong to the system that acts on this.
//
// The book indexes by "Type or Hit Dice of Undead". This uses hit dice, which
// is the general half of that rule and needs no entry per monster - a new
// undead is placed by its own HD with nothing to update here.
//
// Usage -- resolving one attempt:
//
//   const TurningResult result = look_up_turning(skeleton.get_hit_dice(),   // 1 HD
//                                                cleric.get_creature_level()); // level 4
//   result.outcome;      // -> TurningOutcome::ALWAYS_TURNED, no roll needed
//   result.rollNeeded;   // -> 0, meaningless unless outcome is ROLL_REQUIRED
//
//   const TurningResult tough = look_up_turning(11, 1);  // a lich, a novice priest
//   tough.outcome;       // -> TurningOutcome::BEYOND_POWER

// What the table says about one priest level against one kind of undead.
enum class TurningOutcome
{
	BEYOND_POWER, // "--" in the book: this priest cannot affect this undead at all
	ROLL_REQUIRED, // a number: 1d20 must meet or beat rollNeeded
	ALWAYS_TURNED, // "T": turned without a roll
	DESTROYED, // "D": the undead is destroyed outright
};

struct TurningResult
{
	TurningOutcome outcome{ TurningOutcome::BEYOND_POWER };
	int rollNeeded{ 0 }; // meaningful only when outcome is ROLL_REQUIRED
};

// The table's own encoding, so the rows below read like the book's page.
// Negative values stand for the book's letters; anything positive is a d20 target.
inline constexpr int TURNING_BEYOND_POWER = -1; // "--"
inline constexpr int TURNING_ALWAYS = -2; // "T"
inline constexpr int TURNING_DESTROY = -3; // "D" and "D*"

// Rows are undead hit dice 1 through 11; columns are priest levels 1 through 14.
// The book's columns 10-11, 12-13 and 14+ are expanded so a level indexes
// directly. "D*" turns extra creatures, which this does not distinguish.
inline constexpr int TURNING_MIN_HIT_DICE = 1;
inline constexpr int TURNING_MAX_HIT_DICE = 11;
inline constexpr int TURNING_MAX_PRIEST_LEVEL = 14;

inline constexpr std::array<std::array<int, TURNING_MAX_PRIEST_LEVEL>, TURNING_MAX_HIT_DICE> TURNING_TABLE = { {
	// priest level:  1   2   3   4   5   6   7   8   9  10  11  12  13  14+
	/*  1 HD */ {   10,  7,  4, -2, -2, -3, -3, -3, -3, -3, -3, -3, -3, -3 },
	/*  2 HD */ {   16, 13, 10,  7,  4, -2, -2, -3, -3, -3, -3, -3, -3, -3 },
	/*  3 HD */ {   19, 16, 13, 10,  7,  4, -2, -2, -3, -3, -3, -3, -3, -3 },
	/*  4 HD */ {   19, 16, 13, 10,  7,  4, -2, -2, -3, -3, -3, -3, -3, -3 },
	/*  5 HD */ {   20, 19, 16, 13, 10,  7,  4, -2, -2, -3, -3, -3, -3, -3 },
	/*  6 HD */ {   -1, -1, 20, 19, 16, 13, 10,  7,  4, -2, -2, -3, -3, -3 },
	/*  7 HD */ {   -1, -1, -1, 20, 19, 16, 13, 10,  7,  4, -2, -2, -3, -3 },
	/*  8 HD */ {   -1, -1, -1, -1, 20, 19, 16, 13, 10,  7,  4, -2, -2, -3 },
	/*  9 HD */ {   -1, -1, -1, -1, -1, 20, 19, 16, 13, 10,  7,  4, -2, -2 },
	/* 10 HD */ {   -1, -1, -1, -1, -1, -1, 20, 19, 16, 13, 10,  7,  4, -2 },
	/* 11 HD */ {   -1, -1, -1, -1, -1, -1, -1, 20, 19, 16, 13, 10,  7,  4 },
} };

// Reads Table 61 for one priest level against one undead's hit dice.
//
// Hit dice above the table's range are treated as its hardest row, and priest
// levels above it as its last column, which is what "11+ HD" and "14+" mean.
// A priest level below 1 can turn nothing.
//
// Example:
//   look_up_turning(1, 4).outcome;     // -> ALWAYS_TURNED, a 4th-level priest and a skeleton
//   look_up_turning(2, 1).rollNeeded;  // -> 16, a novice priest against a ghoul
//   look_up_turning(11, 3).outcome;    // -> BEYOND_POWER, a lich is out of reach
[[nodiscard]] constexpr TurningResult look_up_turning(int undeadHitDice, int priestLevel)
{
	if (priestLevel < 1)
	{
		return TurningResult{ TurningOutcome::BEYOND_POWER, 0 };
	}

	// The book's last row and column absorb everything beyond them.
	const int row = (undeadHitDice < TURNING_MIN_HIT_DICE)
		? TURNING_MIN_HIT_DICE
		: (undeadHitDice > TURNING_MAX_HIT_DICE ? TURNING_MAX_HIT_DICE : undeadHitDice);
	const int column = (priestLevel > TURNING_MAX_PRIEST_LEVEL) ? TURNING_MAX_PRIEST_LEVEL : priestLevel;

	const int entry = TURNING_TABLE[static_cast<size_t>(row - 1)][static_cast<size_t>(column - 1)];

	switch (entry)
	{
	case TURNING_BEYOND_POWER:
	{
		return TurningResult{ TurningOutcome::BEYOND_POWER, 0 };
	}
	case TURNING_ALWAYS:
	{
		return TurningResult{ TurningOutcome::ALWAYS_TURNED, 0 };
	}
	case TURNING_DESTROY:
	{
		return TurningResult{ TurningOutcome::DESTROYED, 0 };
	}
	}

	return TurningResult{ TurningOutcome::ROLL_REQUIRED, entry };
}

// The toughest undead this priest can affect at all, in hit dice, or zero when
// none. Rises as the priest gains levels, which is the capability worth telling
// the player about - the target numbers improve almost every level, so
// announcing every improvement would announce nearly every level.
//
// Example:
//   highest_turnable_hit_dice(1);  // -> 5
//   highest_turnable_hit_dice(3);  // -> 6, wraiths are newly in reach
//   highest_turnable_hit_dice(0);  // -> 0
[[nodiscard]] constexpr int highest_turnable_hit_dice(int priestLevel)
{
	int highest = 0;

	for (int hitDice = TURNING_MIN_HIT_DICE; hitDice <= TURNING_MAX_HIT_DICE; ++hitDice)
	{
		if (look_up_turning(hitDice, priestLevel).outcome != TurningOutcome::BEYOND_POWER)
		{
			highest = hitDice;
		}
	}

	return highest;
}
