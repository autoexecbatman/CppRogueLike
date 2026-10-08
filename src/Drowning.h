#pragma once

// file: Drowning.h
//
// How long a creature that breathes air lasts underwater, and how its chances fall the
// longer it stays. The rules are the Player's Handbook's, PDF pages 239 and 240 of the
// archive, under "Holding Your Breath".
//
// Nothing stops a creature entering water: can_walk tests wall, closed door, actor and
// decoration, and takes no creature, so it cannot ask who is walking. These apply to
// whatever is standing in it, whether it meant to be or not. They do not apply to a
// creature that breathes water, to one wearing a helm of underwater action, which keeps
// a globe of air about the head, or to one crossing on wings rather than through the
// water.
//
// What is kept out of water is placement: nothing is generated standing in it.
//
// Usage:
//
//   // A Constitution of 14 buys five rounds, rounded up from 4.67.
//   breath_rounds(14);                     // -> 5
//
//   // Past that, a Constitution check each round, worsening by two each time.
//   breath_check_penalty(0);               // -> 0, the first check is unmodified
//   breath_check_penalty(1);               // -> -2
//   breath_check_penalty(3);               // -> -6
//
// The penalty is worked out from how many rounds the clock is past the allowance, so
// nothing counts the checks: the clock already knows.

#include <algorithm>

// Rounds a creature can hold its breath, from its Constitution. "A character can hold
// his breath up to 1/3 his Constitution score in rounds (rounded up)", and "all
// characters are able to hold their breath for one round, regardless of circumstances"
// - so the floor is one however feeble the creature.
//
// Example:
//   breath_rounds(18);   // -> 6
//   breath_rounds(14);   // -> 5, rounded up from 4.67
//   breath_rounds(3);    // -> 1, the floor rather than 1/3 of 3
//   breath_rounds(0);    // -> 1, the floor again
[[nodiscard]] constexpr int breath_rounds(int constitution)
{
	// Rounded up, which is the book's word, so integer division adds two first.
	const int third = (constitution + 2) / 3;
	return std::max(1, third);
}

// What a Constitution check suffers on the nth round past the allowance, counting the
// first such round as zero. "The first check has no modifiers, but each subsequent
// check suffers a -2 cumulative penalty."
//
// Example:
//   breath_check_penalty(0);   // ->  0
//   breath_check_penalty(1);   // -> -2
//   breath_check_penalty(5);   // -> -10
[[nodiscard]] constexpr int breath_check_penalty(int roundsPastAllowance)
{
	return -2 * std::max(0, roundsPastAllowance);
}

// end of file: Drowning.h
