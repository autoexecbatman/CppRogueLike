#pragma once

#include <vector>

class RandomDice;
class Creature;
struct GameContext;

// Everything this header declares lives here rather than in the global namespace,
// which is the rule for a free function crossing translation units.
namespace Turning
{

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

// How many undead one successful turn affects. Each die is rolled on its own and the
// results summed, so the answer runs from 2 to 12 and sits on 7 more often than on
// either end. Rolling the count and the size as a range instead gives a flat 2 to 6,
// where a seventh undead is unreachable.
//
// Example, with two sixes on the dice:
//   roll_turning_cap(dice);   // -> 12, the most a priest can affect at once
[[nodiscard]] int roll_turning_cap(RandomDice& dice);

// How far a priest's presence reaches, in tiles.
//
// The book states no reach at all, which was checked across the whole 2e archive
// rather than assumed: the only spatial rules it gives are about the aftermath -
// free-willed undead flee "until out of his sight", and the turning breaks if they
// are forced closer than ten feet (PDF pages 209 and 726). A roguelike needs a bound
// or an attempt reaches every undead on the level, so this radius is **this game's
// rule, not the book's**, and it is a stated deviation.
//
// Chebyshev distance, so the reach is a square: an undead on the diagonal is as near
// as one straight ahead.
inline constexpr int TURN_UNDEAD_RANGE = 4;

// Attempts to turn every living undead within TURN_UNDEAD_RANGE of the priest,
// resolving one 1d20 for the whole attempt and reading it per creature. Mutates:
// turned creatures gain IS_FLEEING, destroyed creatures are killed.
//
// Reach is distance alone. **A wall between priest and undead does not stop it** -
// nothing here consults the map or the field of view, which TurnUndeadTest pins so
// that adding line of sight has to be a decision rather than a drift. The weakest
// undead are taken first, because the book's 2d6 cap bites from the bottom.
//
// Returns what happened rather than a bare success flag: an attempt can turn
// some undead, destroy others and fail against the rest, all on one roll.
[[nodiscard]] TurnUndeadReport turn_undead(Creature& priest, GameContext& ctx);

} // namespace Turning
