#pragma once

#include <format>
#include <stdexcept>
#include <string_view>

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

// The name a save carries for this class.
//
// Deliberately without a default case, so adding a CreatureClass warns here and in the
// parser below under -Wswitch. No build passes -Werror, so it is a warning rather than
// a refusal.
//
// Example:
//   encode_creature_class(CreatureClass::CLERIC);  // -> "cleric"
[[nodiscard]] inline constexpr std::string_view encode_creature_class(CreatureClass creatureClass)
{
	switch (creatureClass)
	{
	case CreatureClass::FIGHTER:
	{
		return "fighter";
	}
	case CreatureClass::ROGUE:
	{
		return "rogue";
	}
	case CreatureClass::CLERIC:
	{
		return "cleric";
	}
	case CreatureClass::WIZARD:
	{
		return "wizard";
	}
	case CreatureClass::MONSTER:
	{
		return "monster";
	}
	}

	return "monster";
}

// The class a record names. Throws naming what it read.
//
// Example:
//   parse_creature_class("wizard");  // -> CreatureClass::WIZARD
//   parse_creature_class("bard");    // throws std::runtime_error
[[nodiscard]] inline CreatureClass parse_creature_class(std::string_view name)
{
	if (name == "fighter")
	{
		return CreatureClass::FIGHTER;
	}
	if (name == "rogue")
	{
		return CreatureClass::ROGUE;
	}
	if (name == "cleric")
	{
		return CreatureClass::CLERIC;
	}
	if (name == "wizard")
	{
		return CreatureClass::WIZARD;
	}
	if (name == "monster")
	{
		return CreatureClass::MONSTER;
	}

	throw std::runtime_error(std::format("unknown creature class '{}'", name));
}

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
