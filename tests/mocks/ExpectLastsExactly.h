#pragma once
// file: ExpectLastsExactly.h
//
// A test assertion that a buff lasts exactly a number of rounds, asked the way the game
// asks it: update_creature_buffs retires what the clock has passed, and has_buff reports
// what is left. "Lasts exactly N rounds from now" means still in force N - 1 rounds from
// now, part of a round included, and gone N rounds from now.
//
// It retires the buff on the way, so read anything else about the buff first.
//
// Example, a three-round blessing granted at the start of the game:
//   buffs.add_buff(cleric, BuffType::BLESS, 0, 3, BuffApplication::ADD, 0);
//   expect_lasts_exactly(buffs, cleric, BuffType::BLESS, 0, 3);   // passes
//   expect_lasts_exactly(buffs, cleric, BuffType::BLESS, 0, 2);   // fails: still in force at 240

#include <gtest/gtest.h>

#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Creature.h"
#include "src/TurnSchedule.h"

inline void expect_lasts_exactly(
	BuffSystem& buffs,
	Creature& creature,
	BuffType type,
	int currentTime,
	int rounds)
{
	ASSERT_GT(rounds, 0) << "a buff lasting no rounds is already over";

	// The last round it covers, part of a round included.
	const int lastRoundCovered = currentTime + (rounds - 1) * TIME_UNITS_PER_ROUND;
	buffs.update_creature_buffs(creature, lastRoundCovered);
	EXPECT_TRUE(buffs.has_buff(creature, type)) << "gone before " << rounds << " rounds had run";

	// One round further is past its end.
	const int pastTheEnd = currentTime + rounds * TIME_UNITS_PER_ROUND;
	buffs.update_creature_buffs(creature, pastTheEnd);
	EXPECT_FALSE(buffs.has_buff(creature, type)) << "still in force after " << rounds << " rounds";
}
