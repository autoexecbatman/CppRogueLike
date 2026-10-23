#pragma once

// file: DamageSource.h
//
// Who dealt a blow, at the grain the experience rule reads it. The DMG pays
// experience for "victory over their foes" (Chapter 8, Group Awards), so a
// death pays the player only when the player dealt it.
//
// Usage: every caller of Creature::take_damage_and_check_death states one.
//
//   target.take_damage_and_check_death(damage, ctx, DamageType::PHYSICAL,
//       attacker.as_damage_source());   // PLAYER or MONSTER, from the dealer
//   creature.take_damage_and_check_death(damage, ctx, DamageType::PHYSICAL,
//       DamageSource::NO_ATTACKER);     // a trap, poison or starvation
//
// It is a value, so a callback that fires later captures it by copy; it never
// points at a dealer who may have died in the same round.

enum class DamageSource
{
	PLAYER,
	MONSTER,
	NO_ATTACKER,
};

// end of file: DamageSource.h
