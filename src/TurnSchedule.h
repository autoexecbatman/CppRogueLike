#pragma once

// file: TurnSchedule.h
//
// The vocabulary of a time-unit turn order. A being does not accumulate energy until
// it may act; instead every action carries a cost in time units, and a being's next
// action is placed that far ahead on one shared clock. The being with the earliest
// next action goes next.
//
// This is the inverse of an energy system: there, time is fixed in segments and the
// energy gained per segment varies, so the loop visits every being every segment.
// Here the action is fixed - one action, one slot - and the time it costs varies, so
// nothing is visited until it acts.
//
// What that buys, and the reason for choosing it: a duration becomes a timestamp
// rather than a counter. A buff records the time it ends at, so nothing has to
// remember to decrement it once per round, and a being that acts twice as often
// cannot run its own buffs down twice as fast.
//
// Usage:
//
//   // An ordinary creature's ordinary action takes a whole round.
//   scaled_cost(TIME_UNITS_PER_ROUND, TIME_UNITS_PER_ROUND);   // -> 120
//
//   // Half the delay: the same action, half the time, so it acts twice as often.
//   scaled_cost(TIME_UNITS_PER_ROUND, TIME_UNITS_PER_ROUND / 2);   // -> 60
//
//   // A duration the book prints in rounds, as the clock reading it ends at.
//   expiry_time(0, 3);            // -> 360, three rounds from the start of the game
//   rounds_remaining(240, 360);   // -> 1, one round of it left to run
//
//   // Which round a point on the clock falls in, counted from one.
//   round_number_at(0);     // -> 1
//   round_number_at(250);   // -> 3
//
// A being's action delay prices every action it takes. An item that makes only walking
// cheaper passes a smaller base cost for a move instead of changing the delay: the two
// are separate on purpose, because the delay reaches everything a being does and a boot
// does not.

#include <cassert>

// The time one ordinary action by an ordinary creature takes. A round is this long,
// which makes a round a unit of the clock rather than a thing anyone counts.
//
// A hundred and twenty rather than a hundred, because every rate the book prints has
// to divide it exactly. Table 58's three attacks in two rounds is a cost of 80; at a
// round of 100 it floors to 66 and pays out four attacks in two rounds instead of
// three, which is not a rounding error but the wrong rate. 120 divides by 2, 3, 4, 5,
// 6, 8, 10 and 12 - and 12 is the book's own unit for a human's movement rate.
//
// An ordinary creature's ordinary action takes exactly this long, so this is also the
// ordinary pace. A creature's pace is a time on this clock and nothing else, which is
// why there is no second constant naming what normal is.
inline constexpr int TIME_UNITS_PER_ROUND = 120;

// What an action nominally costing `baseCost` actually costs a being whose own ordinary
// action takes `actionDelay`. A being that acts more often has a smaller delay and so
// pays less for the same action, which is the whole of what pace does - and it reaches
// every action rather than only walking.
//
// Both arguments are times on the one clock, so there is no second unit to convert
// through: a round priced against a round is a round.
//
// Integer division, so a cost is rounded down and a very quick being gains a little
// over the exact ratio. That is deliberate: the alternative is a float on the clock,
// and two beings whose next action times differ by a rounding error would reorder
// unpredictably.
//
// Example:
//   scaled_cost(120, 120);   // -> 120, an ordinary action at an ordinary pace
//   scaled_cost(120, 60);    // -> 60, half the delay, so twice as often
//   scaled_cost(120, 240);   // -> 240, twice the delay, so half as often
//   scaled_cost(80, 60);     // -> 40, a 3/2 attack rate at twice the pace
//   scaled_cost(180, 120);   // -> 180, a move that takes half again as long
[[nodiscard]] constexpr int scaled_cost(int baseCost, int actionDelay)
{
	assert(actionDelay >= 1 && "scaled_cost: a delay of zero would act without end");
	assert(baseCost >= 0 && "scaled_cost: an action cannot give time back");

	return baseCost * actionDelay / TIME_UNITS_PER_ROUND;
}

// How many times a being acts before the clock reaches a limit, given where it
// stands and what one action costs it. This is the whole of the turn order: both
// directions fall out of one expression, with no branch for fast or slow.
//
// A being standing at 0 whose action costs a whole round acts once before the next
// round. One whose action costs half a round acts twice - the quickling. One whose
// action costs two rounds acts once and is then standing past the next limit, so it
// acts not at all the turn after - the snail skipping.
//
// Zero when the being is already standing at or past the limit, which is what a skip
// is: nothing special happens, there is simply no action in the window.
//
// Example, a limit one round ahead of a being standing at the start:
//   actions_before(0, 100, 100);   // -> 1, an ordinary creature
//   actions_before(0, 50, 100);    // -> 2, twice as fast
//   actions_before(0, 25, 100);    // -> 4, four times as fast
//   actions_before(0, 200, 100);   // -> 1, slow, and it has now overshot
//   actions_before(200, 200, 200); // -> 0, the overshot being skips this one
[[nodiscard]] constexpr int actions_before(int nextActionTime, int actionCost, int clockLimit)
{
	assert(actionCost >= 1 && "actions_before: a free action would act forever");

	// Already standing at or past the limit, so this window holds nothing for it.
	const int remaining = clockLimit - nextActionTime;
	if (remaining <= 0)
	{
		return 0;
	}

	// Rounded up, because a being standing before the limit acts once more even when
	// the time left is less than a whole action: it is due, and being due is enough.
	return (remaining + actionCost - 1) / actionCost;
}

// How many whole rounds the clock has finished. Distinct from the round in progress:
// at time zero no round has finished and the first one is being played, so this reads
// zero where round_number_at reads one. Upkeep runs per finished round, which is what
// stops a being that acts twice in a round feeling the round twice.
//
// Example:
//   rounds_completed_at(0);     // -> 0
//   rounds_completed_at(119);   // -> 0
//   rounds_completed_at(120);   // -> 1
//   rounds_completed_at(250);   // -> 2
[[nodiscard]] constexpr int rounds_completed_at(int timeUnits)
{
	assert(timeUnits >= 0 && "rounds_completed_at: the clock starts at zero and only rises");

	return timeUnits / TIME_UNITS_PER_ROUND;
}

// Which round a point on the clock falls in, counted from one so the first round of a
// game is round 1 - the convention apply_round_upkeep already reads the clock by.
//
// Example:
//   round_number_at(0);     // -> 1
//   round_number_at(119);   // -> 1
//   round_number_at(120);   // -> 2
//   round_number_at(250);   // -> 3
[[nodiscard]] constexpr int round_number_at(int timeUnits)
{
	assert(timeUnits >= 0 && "round_number_at: the clock starts at zero and only rises");

	return timeUnits / TIME_UNITS_PER_ROUND + 1;
}

// The clock reading at which a duration printed in rounds runs out. Durations are the
// reason this is a time-unit order rather than an energy one: a thing that records the
// time it ends at cannot be ticked, so a creature acting twice in a round cannot run
// its own buffs down twice as fast, and nothing has to remember to tick it at all.
//
// A duration of zero rounds ends at the reading it was given, so it is already over -
// which is what a spell with no duration left should be.
//
// Example, at the start of the game and six hundred units in:
//   expiry_time(0, 3);     // -> 360
//   expiry_time(600, 3);   // -> 960
//   expiry_time(0, 0);     // -> 0, over the moment it was granted
[[nodiscard]] constexpr int expiry_time(int currentTime, int durationRounds)
{
	assert(currentTime >= 0 && "expiry_time: the clock starts at zero and only rises");
	assert(durationRounds >= 0 && "expiry_time: a duration cannot run backwards");

	return currentTime + durationRounds * TIME_UNITS_PER_ROUND;
}

// Whole rounds a duration has left, for printing a count the book's reader recognises.
// Rounded up, so it reads zero only once the duration is over: a buff still in force
// with half a round to run reports one, because reporting none would read as expired.
//
// Example, a three-round blessing granted at the start of the game:
//   rounds_remaining(0, 360);     // -> 3
//   rounds_remaining(240, 360);   // -> 1
//   rounds_remaining(359, 360);   // -> 1, the last of it
//   rounds_remaining(360, 360);   // -> 0, and only here
[[nodiscard]] constexpr int rounds_remaining(int currentTime, int expiryTime)
{
	assert(currentTime >= 0 && "rounds_remaining: the clock starts at zero and only rises");

	const int remaining = expiryTime - currentTime;
	if (remaining <= 0)
	{
		return 0;
	}

	return (remaining + TIME_UNITS_PER_ROUND - 1) / TIME_UNITS_PER_ROUND;
}
