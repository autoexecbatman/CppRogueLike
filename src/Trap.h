// file: Trap.h
// Trap mechanics: pit, dart, arrow hazards placed in dungeons
// Inherits from TileFeature; blocks movement on trigger; applies damage to creatures

#pragma once

#include "TileFeature.h"
#include "Vector2D.h"
#include "Renderer.h" // TileRef, held by value below

class RandomDice;

enum class TrapType
{
	PIT,
	DART,
	ARROW
};

enum class TrapState
{
	HIDDEN,     // Player hasn't detected it yet
	DETECTED,   // Player knows it's here (via DEX check or explicit detection)
	TRIGGERED,  // Trap has been sprung (may destroy or reset)
	DISARMED    // Trap has been disarmed, is inert
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

	// Disarm attempt: the creature rolls 1d20 plus its dexterity modifier
	// against the trap's disarm DC. Failure sets the trap off. Traps are the
	// only feature that answers this, which is why it is not on TileFeature.
	DisarmResult attempt_disarm(Creature& creature, GameContext& ctx);

	// Destroy this trap (called after trigger or successful disarm)
	void destroy();

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
	// What 1d20 plus the dexterity modifier must reach to spot the trap, and to disarm it.
	int detectionDC{ 15 };
	int disarmDC{ 12 };

	// The detection roll a hidden trap gets as a creature steps onto it: 1d20 plus
	// the dexterity modifier against detectionDC, success leaving it DETECTED.
	// Requires the trap to be hidden.
	void attempt_passive_detection(Creature& creature, GameContext& ctx);

	// Get damage dice roll result
	int roll_damage(RandomDice& dice) const;
};
