// file: Trap.h
// Trap mechanics: pit, dart, arrow hazards placed in dungeons
// Inherits from TileFeature; blocks movement on trigger; applies damage to creatures

#pragma once

#include "../Actor/TileFeature.h"
#include "../Utils/Vector2D.h"

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
	Trap(Vector2D position, TrapType type, const TileConfig& tileConfig);

	// Trap properties
	TrapType get_type() const { return type_; }
	TrapState get_state() const { return state_; }
	int get_damage_dice_count() const { return damageDiceCount_; }
	int get_damage_dice_size() const { return damageDiceSize_; }

	// Detection: player makes DEX check to spot the trap
	// Returns true if trap is now detected (was hidden, now revealed)
	bool attempt_detect(Creature& creature, GameContext& ctx);

	// Applies the trap's effect to a creature entering its tile.
	EntryResult on_creature_enter(Creature& creature, GameContext& ctx) override;

	// Disarm attempt: the creature rolls 1d20 plus its dexterity modifier
	// against the trap's disarm DC. Failure sets the trap off. Traps are the
	// only feature that answers this, which is why it is not on TileFeature.
	DisarmResult attempt_disarm(Creature& creature, GameContext& ctx);

	// Destroy this trap (called after trigger or successful disarm)
	void destroy();

private:
	TrapType type_;
	TrapState state_;
	int damageDiceCount_;   // Number of dice (e.g., 2 for pit = 2d6)
	int damageDiceSize_;    // Dice size (6 for 2d6, 4 for 1d4)
	int detectionDC_;       // DC for passive detection (typically 15)
	int disarmDC_;          // DC for disarm attempt (typically 12)

	// Internal: handle detection during movement
	void attempt_passive_detection(Creature& creature, GameContext& ctx);

	// Get damage dice roll result
	int roll_damage(RandomDice& dice) const;
};
