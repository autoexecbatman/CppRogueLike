#pragma once

// file: CreatureClass.h
//
// The AD&D 2e class a creature belongs to. Kept apart from Creature so a
// header that only needs the class - level-up progression, a UI panel, a
// factory - can take it without the whole creature definition.
//
// MONSTER is the answer for anything that is not a player character. It is a
// real member rather than an absence: monsters advance and fight by their own
// rules, and code that switches on class must handle them.
//
// Usage:
//
//   if (creature.get_creature_class() == CreatureClass::CLERIC)  // priests only
//   {
//       turn_undead(creature, ctx);
//   }

enum class CreatureClass
{
	FIGHTER,
	ROGUE,
	CLERIC,
	WIZARD,
	MONSTER,
};

// Whether a class advances on the warrior tables - the constitution column with
// the bonus above +2, and the warrior hit dice. Fighter is the only warrior the
// game has; a paladin or ranger joins here, and every warrior rule follows.
//
// Example:
//   is_warrior(CreatureClass::FIGHTER);  // -> true
//   is_warrior(CreatureClass::WIZARD);   // -> false
[[nodiscard]] inline constexpr bool is_warrior(CreatureClass creatureClass)
{
	return creatureClass == CreatureClass::FIGHTER;
}
