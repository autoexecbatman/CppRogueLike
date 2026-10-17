// file: Trap.h
// Trap mechanics: pit, dart, arrow hazards placed in dungeons
// Inherits from TileFeature; blocks movement on trigger; applies damage to creatures
//
// A trap starts hidden and is found only by searching for it, which is the player's
// `f` key and TrapSearch::search_around below. Nothing finds one by walking onto it.
//
//   TrapSearch::search_around(thief, ctx);   // -> 7, rounds spent; "You find a hidden dart trap."
//   TrapSearch::search_around(fighter, ctx); // -> nullopt, no turn spent

#pragma once

#include <optional>

#include "Persistent.h" // json
#include "Renderer.h" // TileRef, held by value below
#include "TileFeature.h"
#include "Vector2D.h"

class Player;
class RandomDice;

enum class TrapType
{
	PIT,
	DART,
	ARROW
};

enum class TrapState
{
	HIDDEN, // Player hasn't found it yet
	DETECTED, // Player knows it's here, found by a search
	TRIGGERED, // Trap has been sprung (may destroy or reset)
	DISARMED // Trap has been disarmed, is inert
};

// What one search did to one trap. Only FOUND changes what the player can see.
enum class TrapSearchResult
{
	NOT_HIDDEN, // already found, sprung or disarmed; nothing to look for
	NO_WAY_TO_FIND, // this searcher has no means of finding this kind of trap
	ALREADY_SEARCHED, // this trap was searched at this experience level already
	FOUND, // the roll succeeded and the trap is now known
	MISSED, // the roll failed; the trap stays hidden until the next level
};

// Trap class - represents environmental hazards (pit, dart, arrow)
class Trap : public TileFeature
{
private:
	TrapType type{ TrapType::PIT };
	TrapState state{ TrapState::HIDDEN };
	// The damage dice, set from the type: a 2d6 pit is count 2, size 6.
	int damageDiceCount{ 0 };
	int damageDiceSize{ 0 };
	// The two faces of a trap: what it shows once spotted, and what it shows
	// once sprung or defused. Both are resolved from the type in the
	// constructor, so the TileConfig is not held beyond it.
	TileRef armedTile{};
	TileRef sprungTile{};

	// The experience level at which this trap was last worked on, 0 for never. The
	// book gives a thief one attempt per level on a given trap, so the trap is what
	// remembers it: a character who fails here may still try the next trap along.
	int disarmAttemptedAtLevel{ 0 };

	// The experience level at which this trap was last searched for, 0 for never. The
	// same once-per-level rule as the disarm, kept on the trap for the same reason.
	int searchedAtLevel{ 0 };

	// Puts the tile and the visibility back in step with the state. Called on load,
	// where the constructor has just set both as though the trap were fresh.
	void apply_state_appearance();

	// Get damage dice roll result
	int roll_damage(RandomDice& dice) const;

public:
	Trap(Vector2D position, TrapType trapType, const TileConfig& tileConfig);

	// Trap properties
	TrapType get_type() const { return type; }

	TrapState get_state() const { return state; }

	int get_damage_dice_count() const { return damageDiceCount; }

	int get_damage_dice_size() const { return damageDiceSize; }

	// Applies the trap's effect to a creature entering its tile. A hidden or known trap
	// springs, on the player and a monster alike, with no roll to notice it: only a
	// search finds a trap. A disarmed one is walked over. Springing deals the damage
	// dice and stops the move.
	EntryResult on_creature_enter(Creature& creature, GameContext& ctx) override;

	// One search of this trap by this character, once per experience level per trap.
	// A pit is a stonework trap, found by a dwarf on 1-3 on 1d6 (PHB page 44); a dart or
	// arrow trap is a small one, found by a thief's Find/Remove Traps on d100 (PHB page
	// 87). Each finds only its own kind, and finding one shows it.
	//
	// Example, a human thief whose Find/Remove Traps is 45, beside a hidden dart trap:
	//   trap.search(thief, ctx);   // d100 40  -> TrapSearchResult::FOUND
	//   trap.search(thief, ctx);   // no roll  -> NOT_HIDDEN
	// The same thief beside a hidden pit:
	//   pit.search(thief, ctx);    // no roll  -> NO_WAY_TO_FIND
	// A dwarf beside it:
	//   pit.search(dwarf, ctx);    // d6 5     -> MISSED
	//   pit.search(dwarf, ctx);    // no roll  -> ALREADY_SEARCHED, same level
	TrapSearchResult search(Player& searcher, GameContext& ctx);

	// One attempt at removing this trap, rolled against the character's own
	// Find/Remove Traps percentage (PHB page 87). Nobody without that skill can
	// try. An ordinary failure leaves the trap exactly as it was and takes the
	// character's attempt for this experience level; only a roll of 96 or more
	// sets it off. Traps are the only feature that answers this, which is why it
	// is not on TileFeature.
	//
	// Example, a thief whose Find/Remove Traps is 45, on a trap it has found:
	//   trap.attempt_disarm(thief, ctx);   // d100 40  -> DisarmResult::DISARMED
	//   trap.attempt_disarm(thief, ctx);   // d100 90  -> BEYOND_SKILL, untouched
	//   trap.attempt_disarm(thief, ctx);   // no roll  -> ALREADY_TRIED, same level
	//   trap.attempt_disarm(thief, ctx);   // d100 97  -> TRIGGERED, and it hurts
	// A fighter standing over the same trap:
	//   trap.attempt_disarm(fighter, ctx); // no roll  -> DisarmResult::NO_SKILL
	DisarmResult attempt_disarm(Player& thief, GameContext& ctx);

	// Destroy this trap (called after trigger or successful disarm)
	void destroy();

	// Writes what cannot be rebuilt from the type: where it is, which kind it is, what
	// state it is in, and the experience levels its last disarm attempt and its last
	// search were made at.
	void save(json& j) override;

	// Restores those five and nothing else. The damage dice, the name, both tiles and
	// the colour come from the constructor, so a save can never disagree with the type
	// table; visibility follows from the state rather than being stored.
	void load(const json& j) override;
};

namespace TrapSearch
{

// The search action: every live trap on the eight tiles around the searcher is
// searched once, a "You find a hidden <trap>." for each one found, and "You find no
// traps." when nothing was. Returns the rounds it took, or nullopt with "You don't
// know how to look for traps." for a character who can find no kind of trap, who
// spends no turn.
//
// Reach and the dwarf's time are this game's rules, stated deviations. The book puts
// a dwarf's detection "within 10 feet" and gives it no time; this game states no
// feet-per-tile scale to convert the distance with (Vision.h, where the torch's 15
// feet is 6 tiles), so the search covers what a thief's touch covers, the adjacent
// tiles, and one search action reaches one area. A dwarf's look takes 1 round. A
// thief's is the book's 1d10 rounds, and a dwarf thief takes the thief's time.
//
// Every trap's roll is made before the 1d10 for the time.
//
// Example, a thief with Find/Remove Traps 45 beside a hidden dart trap:
//   TrapSearch::search_around(thief, ctx);   // d100 40, d10 7 -> 7; the trap is found
// A dwarf fighter beside nothing:
//   TrapSearch::search_around(dwarf, ctx);   // no roll        -> 1; "You find no traps."
// A human fighter:
//   TrapSearch::search_around(fighter, ctx); // no roll        -> nullopt
[[nodiscard]] std::optional<int> search_around(Player& searcher, GameContext& ctx);

} // namespace TrapSearch

namespace TrapDisarm
{

// The disarm action on one tile: works on the live trap there, says what came of it,
// and returns the rounds it took, or nullopt for a refusal that made no roll - nothing
// there, a trap not yet found, one already disarmed, no skill, or this level's attempt
// already spent. An attempt that rolled, whatever it rolled, takes the book's 1d10
// rounds (PHB page 87), rolled after the attempt.
//
// Example, a thief with Find/Remove Traps 45 facing a found dart trap to the east:
//   TrapDisarm::disarm_at(thief, eastTile, ctx);   // d100 40, d10 7 -> 7; "You successfully disarm the trap."
//   TrapDisarm::disarm_at(thief, eastTile, ctx);   // no roll        -> nullopt; "That trap is already disarmed."
// Facing bare floor:
//   TrapDisarm::disarm_at(thief, westTile, ctx);   // no roll        -> nullopt; "There is no trap there."
[[nodiscard]] std::optional<int> disarm_at(Player& thief, Vector2D target, GameContext& ctx);

} // namespace TrapDisarm

// end of file: Trap.h
