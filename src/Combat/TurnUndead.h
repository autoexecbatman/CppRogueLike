#pragma once

#include <vector>

// file: TurnUndead.h
//
// AD&D 2e turning: a priest channels their deity's power to drive off or destroy
// undead. Table 61 itself lives in TurningTable.h; this decides who may attempt
// it, which creatures are affected, and what happens to each.
//
// The book's rules this implements: one 1d20 for the whole attempt, read
// separately per undead type; a turned undead flees; a "D" result destroys it;
// at most 2d6 undead are affected, weakest first.
//
// Deliberately not implemented, and each needs a rule of its own: paladins
// turning as priests two levels lower, evil priests commanding rather than
// turning, and the once-per-encounter limit, which needs an encounter concept
// this game does not have.
//
// Usage -- resolving the player's turn attempt:
//
//   const TurnUndeadReport report = turn_undead(cleric, ctx);   // rolls 1d20 once
//   report.attempted;   // -> false if the creature is not a priest
//   report.roll;        // -> 12, the single d20 the whole attempt used
//   report.turned;      // -> creatures now fleeing
//   report.destroyed;   // -> creatures reduced to nothing

class Creature;
struct GameContext;

// What one turning attempt did. Named creatures rather than counts, so the
// caller can say which undead fled without re-deriving it.
struct TurnUndeadReport
{
	bool attempted{ false }; // false when the creature cannot turn at all
	int roll{ 0 }; // the single d20 shared by every type this attempt
	std::vector<Creature*> turned{};
	std::vector<Creature*> destroyed{};
	std::vector<Creature*> resisted{};
};

// The book affects 2d6 undead per successful turn.
inline constexpr int TURN_UNDEAD_DICE_COUNT = 2;
inline constexpr int TURN_UNDEAD_DICE_SIDES = 6;

// How far a priest's presence reaches. The book leaves this to line of sight;
// this game bounds it by the player's own field of view radius.
inline constexpr int TURN_UNDEAD_RANGE = 4;

// Attempts to turn every undead the priest can see, resolving one 1d20 for the
// whole attempt and reading it per creature. Mutates: turned creatures gain
// IS_FLEEING, destroyed creatures are killed.
//
// Returns what happened rather than a bare success flag: an attempt can turn
// some undead, destroy others and fail against the rest, all on one roll.
[[nodiscard]] TurnUndeadReport turn_undead(Creature& priest, GameContext& ctx);
