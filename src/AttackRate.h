#pragma once

// file: AttackRate.h
//
// What an attack rate costs on the clock. Table 58 gives a warrior three attacks
// every two rounds from 7th level and two attacks every round from 13th (Player's
// Handbook, PDF page 231), and a time-unit turn order spends that as a price rather
// than handing out swings: an attack costing two thirds of a round pays out three
// attacks every two rounds by itself.
//
// One attack is one action. Nothing has to decide whether a round is the odd one, a
// character that spends rounds walking arrives with nothing saved up and nothing
// owed, and a rate the book never printed - 1.73 attacks a round - is a cost like
// any other rather than a case to handle.
//
// Usage - one attack, charged for what it took:
//
//   player.attacker->attack(target, AttackKind::MELEE, ctx);
//   spend_player_action(ctx, attack_cost(player.get_attacks_per_round()));

#include <cassert>

#include "TurnSchedule.h"

// What one attack costs on the clock, in time units, for a character with this many
// attacks per round. A whole round divided by the rate: one attack a round costs a
// round, three-halves costs two thirds of one, two costs half of one.
//
// Floors at one time unit, because an attack that cost nothing would repeat forever.
//
// Example, the three rates Table 58 gives a character:
//   attack_cost(1.0f);   // -> 100
//   attack_cost(1.5f);   // -> 66
//   attack_cost(2.0f);   // -> 50
[[nodiscard]] constexpr int attack_cost(float attacksPerRound)
{
	assert(attacksPerRound > 0.0f && "attack_cost: a creature that never attacks has no attack to cost");

	const int cost = static_cast<int>(static_cast<float>(TIME_UNITS_PER_ROUND) / attacksPerRound);
	return cost >= 1 ? cost : 1;
}
