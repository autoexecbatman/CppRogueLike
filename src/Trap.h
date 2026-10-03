// file: Trap.h
// Trap mechanics: pit, dart, arrow hazards placed in dungeons
// Inherits from TileFeature; blocks movement on trigger; applies damage to creatures

#pragma once

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
	HIDDEN, // Player hasn't detected it yet
	DETECTED, // Player knows it's here (via DEX check or explicit detection)
	TRIGGERED, // Trap has been sprung (may destroy or reset)
	DISARMED // Trap has been disarmed, is inert
};

// Trap class - represents environmental hazards (pit, dart, arrow)
class Trap : public TileFeature
{
public:
	Trap(Vector2D position, TrapType trapType, const TileConfig& tileConfig);

	// Trap properties
	TrapType get_type() const { return type; }
	TrapState get_state() const { return state; }
	int get_damage_dice_count() const { return damageDiceCount; }
	int get_damage_dice_size() const { return damageDiceSize; }

	// Detection: player makes DEX check to spot the trap
	// Returns true if trap is now detected (was hidden, now revealed)
	bool attempt_detect(Creature& creature, GameContext& ctx);

	// Applies the trap's effect to a creature entering its tile. A hidden trap gets
	// one detection roll: missed, it springs; noticed, the creature stops short and it
	// stays armed. A known or triggered trap springs with no roll; a disarmed one is
	// walked over. Springing deals the damage dice and stops the move.
	EntryResult on_creature_enter(Creature& creature, GameContext& ctx) override;

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
	//   trap.attempt_disarm(thief, ctx);   // no roll  -> BEYOND_SKILL, same level
	//   trap.attempt_disarm(thief, ctx);   // d100 97  -> TRIGGERED, and it hurts
	// A fighter standing over the same trap:
	//   trap.attempt_disarm(fighter, ctx); // no roll  -> DisarmResult::NO_SKILL
	DisarmResult attempt_disarm(Player& thief, GameContext& ctx);

	// Destroy this trap (called after trigger or successful disarm)
	void destroy();

	// Writes what cannot be rebuilt from the type: where it is, which kind it is, what
	// state it is in, and the experience level its last disarm attempt was made at.
	void save(json& j) override;

	// Restores those four and nothing else. The damage dice, the name, both tiles and
	// the colour come from the constructor, so a save can never disagree with the type
	// table; visibility follows from the state rather than being stored.
	void load(const json& j) override;

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
	// What 1d20 plus the dexterity modifier must reach to spot the trap. Removing
	// one is a percentile skill instead, so it has no such number.
	int detectionDC{ 15 };

	// The experience level at which this trap was last worked on, 0 for never. The
	// book gives a thief one attempt per level on a given trap, so the trap is what
	// remembers it: a character who fails here may still try the next trap along.
	int disarmAttemptedAtLevel{ 0 };

	// Puts the tile and the visibility back in step with the state. Called on load,
	// where the constructor has just set both as though the trap were fresh.
	void apply_state_appearance();

	// The detection roll a hidden trap gets as a creature steps onto it: 1d20 plus
	// the dexterity modifier against detectionDC, success leaving it DETECTED.
	// Requires the trap to be hidden.
	void attempt_passive_detection(Creature& creature, GameContext& ctx);

	// Get damage dice roll result
	int roll_damage(RandomDice& dice) const;
};
