#pragma once

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "DiceExpr.h"
#include "RandomDice.h"

// Damage type classification for resistance calculations
enum class DamageType
{
	PHYSICAL, // Normal weapon damage
	FIRE, // Fire damage
	COLD, // Cold/ice damage
	LIGHTNING, // Lightning/shock damage
	POISON, // Poison damage
	ACID, // Acid damage
	MAGIC, // Pure magical damage
};

// What a damage type is called: the name a save carries for it, and the one shown in
// the resolver's log and the monster editor's field. One table, so a record and a label
// cannot drift apart - and changing a string here changes what a save means, since
// parse_damage_type below is its inverse.
//
// Example:
//   damage_type_name(DamageType::FIRE);   // -> "fire"
inline std::string_view damage_type_name(DamageType damageType)
{
	switch (damageType)
	{
	case DamageType::PHYSICAL:
	{
		return "physical";
	}
	case DamageType::FIRE:
	{
		return "fire";
	}
	case DamageType::COLD:
	{
		return "cold";
	}
	case DamageType::LIGHTNING:
	{
		return "lightning";
	}
	case DamageType::POISON:
	{
		return "poison";
	}
	case DamageType::ACID:
	{
		return "acid";
	}
	case DamageType::MAGIC:
	{
		return "magic";
	}
	}
	// Every type returns above; a new one fails to compile under -Wswitch
	// rather than falling through to a name that belongs to nothing.
	std::unreachable();
}

// The damage type a record names, the inverse of damage_type_name. Throws naming what
// it read, so a record written by a build that knew a type this one does not is refused
// rather than cast to whichever type the number lands on.
//
// Example:
//   parse_damage_type("fire");    // -> DamageType::FIRE
//   parse_damage_type("sonic");   // throws std::runtime_error
inline DamageType parse_damage_type(std::string_view name)
{
	if (name == "physical")
	{
		return DamageType::PHYSICAL;
	}
	if (name == "fire")
	{
		return DamageType::FIRE;
	}
	if (name == "cold")
	{
		return DamageType::COLD;
	}
	if (name == "lightning")
	{
		return DamageType::LIGHTNING;
	}
	if (name == "poison")
	{
		return DamageType::POISON;
	}
	if (name == "acid")
	{
		return DamageType::ACID;
	}
	if (name == "magic")
	{
		return DamageType::MAGIC;
	}

	throw std::runtime_error(std::format("unknown damage type '{}'", name));
}

// The next type in the editor's cycle, wrapping at the end, so every type is
// reachable by pressing one key.
//
// Example:
//   next_damage_type(DamageType::PHYSICAL);   // -> DamageType::FIRE
//   next_damage_type(DamageType::MAGIC);      // -> DamageType::PHYSICAL
inline DamageType next_damage_type(DamageType damageType)
{
	switch (damageType)
	{
	case DamageType::PHYSICAL:
	{
		return DamageType::FIRE;
	}
	case DamageType::FIRE:
	{
		return DamageType::COLD;
	}
	case DamageType::COLD:
	{
		return DamageType::LIGHTNING;
	}
	case DamageType::LIGHTNING:
	{
		return DamageType::POISON;
	}
	case DamageType::POISON:
	{
		return DamageType::ACID;
	}
	case DamageType::ACID:
	{
		return DamageType::MAGIC;
	}
	case DamageType::MAGIC:
	{
		return DamageType::PHYSICAL;
	}
	}
	std::unreachable();
}

// Result of applying temporary HP shield to damage
struct ShieldResult
{
	int damageAfterShield;
	int tempHpAfterShield;
};

// A damage roll: its dice, its type, and the range derived from the dice. The
// dice are the fact - fire resistance takes two off every die and lets none
// fall below one, which no range can express - so a DamageInfo is built from
// a dice expression and its range is never written by hand.
//
// Example:
//   DamageInfo{ "3d12", DamageType::FIRE }.dice;   // -> { 3, 12, 0 }
//   DamageInfo{ "1d12+5", DamageType::FIRE }.dice; // -> { 1, 12, 5 }
struct DamageInfo
{
	DiceExpr dice; // The dice the display names; what a per-die rule acts on
	int minDamage; // Derived from the dice
	int maxDamage; // Derived from the dice
	std::string displayRoll; // The dice as written: "1d8", "1d6+1", etc.
	DamageType damageType; // Type of damage for resistance calculations

	// Constructors
	DamageInfo()
		: dice{ 1, 2, 0 }, minDamage(1), maxDamage(2), displayRoll("1d2"), damageType(DamageType::PHYSICAL) {}

	// From the dice alone: the display and the range are derived, so there is no
	// second copy of either to disagree. This is what an editor holds - dice -
	// and what the data file stores is the text of them.
	//
	// Example:
	//   DamageInfo{ DiceExpr{ 2, 4, 0 }, DamageType::FIRE }.displayRoll;   // -> "2d4"
	//   DamageInfo{ DiceExpr{ 2, 4, 0 }, DamageType::FIRE }.maxDamage;     // -> 8
	DamageInfo(const DiceExpr& rolled, DamageType type)
		: dice(rolled)
		, minDamage(dice.min_total())
		, maxDamage(dice.max_total())
		, displayRoll(to_text(dice))
		, damageType(type)
	{
	}

	// From the text the data file stores.
	//
	// Example:
	//   DamageInfo{ "2d4", DamageType::FIRE }.minDamage;   // -> 2
	//   DamageInfo{ "2d4", DamageType::FIRE }.maxDamage;   // -> 8
	DamageInfo(const std::string& display, DamageType type)
		: DamageInfo(parse_dice_expression(display), type)
	{
	}

	// Rolls every die and adds the bonus once, so 2d6 lands on 7 more often than on 2.
	// A fixed value, having no dice, is its bonus.
	//
	// Example, dice forced to 6 and 6:
	//   DamageInfo{ "2d6", DamageType::PHYSICAL }.roll_damage(&rng);   // -> 12
	//   DamageInfo{ "5", DamageType::PHYSICAL }.roll_damage(&rng);     // -> 5
	int roll_damage(RandomDice* rng) const
	{
		return roll_dice(rng, dice);
	}

	int get_average_damage() const { return (minDamage + maxDamage) / 2; }

	// Modification operations
	// The same damage with its bonus moved, as a new value: the dice carry the
	// bonus and the range and the text are derived from them, so a second call
	// reads as one sum rather than as a string of them. The type is unchanged -
	// a bonus says how hard a weapon hits, never what kind of damage it deals.
	//
	// Example:
	//   const DamageInfo sword{ "1d8", DamageType::PHYSICAL };
	//   sword.with_enhancement(2).with_enhancement(3).displayRoll;   // -> "1d8+5"
	[[nodiscard]] DamageInfo with_enhancement(int damage_bonus) const
	{
		return DamageInfo{ DiceExpr{ dice.num, dice.sides, dice.bonus + damage_bonus }, damageType };
	}

	// Utility functions
	bool is_valid() const { return minDamage > 0 && maxDamage >= minDamage; }
	std::string get_damage_range() const
	{
		if (minDamage == maxDamage)
		{
			return std::format("{}", minDamage);
		}
		return std::format("{}-{}", minDamage, maxDamage);
	}

	// Comparison operators
	bool operator==(const DamageInfo& other) const
	{
		return minDamage == other.minDamage && maxDamage == other.maxDamage;
	}

	bool operator!=(const DamageInfo& other) const { return !(*this == other); }
};

// Common damage values for easy reference
namespace DamageValues
{
inline DamageInfo Unarmed()
{
	return { "1d2", DamageType::PHYSICAL };
}
inline DamageInfo Dagger()
{
	return { "1d4", DamageType::PHYSICAL };
}
inline DamageInfo ShortSword()
{
	return { "1d6", DamageType::PHYSICAL };
}
inline DamageInfo LongSword()
{
	return { "1d8", DamageType::PHYSICAL };
}
inline DamageInfo GreatSword()
{
	return { "1d10", DamageType::PHYSICAL };
}
inline DamageInfo BattleAxe()
{
	return { "1d8", DamageType::PHYSICAL };
}
inline DamageInfo WarHammer()
{
	return { "1d4+1", DamageType::PHYSICAL };
}
inline DamageInfo Staff()
{
	return { "1d6", DamageType::PHYSICAL };
}
inline DamageInfo LongBow()
{
	return { "1d6", DamageType::PHYSICAL };
}
} // namespace DamageValues
