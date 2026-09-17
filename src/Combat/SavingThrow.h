#pragma once

// file: Combat/SavingThrow.h
//
// Saving throws, AD&D 2nd edition Player's Handbook Table 60. A target is a
// pure function of the creature's class group, its level and the category of
// the effect, so it is read from the table when asked and never stored on a
// creature - the level is the only input it needs.
//
// Usage:
//
//   // The number a 1st-level fighter must roll to resist a spell.
//   SavingThrows::target(CreatureClass::FIGHTER, 1, SavingThrow::SPELL);   // -> 17
//
//   // Whether this creature resisted, rolling a d20 against that target.
//   // The modifier is what the effect grants: a ring of fire resistance is +4
//   // against magical fire, its wearer's helm of brilliance +8.
//   if (SavingThrows::is_made(target, SavingThrow::SPELL, 0, ctx))
//   {
//       // half damage, no effect, whatever the spell says a save means
//   }
//
//   // Whether reaching this level moves the creature onto a better row.
//   SavingThrows::improves_at(CreatureClass::WIZARD, 6);   // -> true

#include "../Actor/CreatureClass.h"

class Creature;
struct GameContext;

// The five categories, in the Player's Handbook's own order of priority: a
// character rolls the highest-priority save that applies, so an effect that is
// both a poison and a spell is a poison save.
enum class SavingThrow
{
	PARALYZATION_POISON_DEATH,
	ROD_STAFF_WAND,
	PETRIFICATION_POLYMORPH,
	BREATH_WEAPON,
	SPELL,
};

namespace SavingThrows
{
	// The number this creature must roll on a d20 to make the save, from
	// Table 60. Monsters save on the warrior rows, as they attack on the
	// warrior table. A level below the table's first row takes its first row.
	//
	// Example:
	//   target(CreatureClass::FIGHTER, 1, SavingThrow::SPELL);   // -> 17
	//   target(CreatureClass::WIZARD, 1, SavingThrow::SPELL);    // -> 12
	[[nodiscard]] int target(CreatureClass creatureClass, int level, SavingThrow category);

	// Whether reaching this level moves the class onto a better row of the
	// table. Derived from the rows rather than from a list of levels, so it
	// cannot drift from them.
	//
	// Example:
	//   improves_at(CreatureClass::WIZARD, 6);    // -> true, the 6-10 row begins
	//   improves_at(CreatureClass::WIZARD, 7);    // -> false, the same row
	[[nodiscard]] bool improves_at(CreatureClass creatureClass, int level);

	// Rolls a d20 for the creature and reports whether it saved. The modifier
	// is what the effect grants the saver - a ring of fire resistance is +4
	// against magical fire - and is added to the roll, as the book adds it "to
	// the die roll".
	//
	// Example, a 1st-level fighter resisting a spell, needing 17:
	//   is_made(fighter, SavingThrow::SPELL, 0, ctx);   // -> true on a 17 or better
	//   is_made(fighter, SavingThrow::SPELL, 4, ctx);   // -> true on a 13 or better
	[[nodiscard]] bool is_made(const Creature& saver, SavingThrow category, int modifier, GameContext& ctx);
}
