#pragma once

#include <format>
#include <string>

#include "../Random/DiceExpr.h"
#include "../Random/RandomDice.h"

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

	// Core damage operations
	int roll_damage(RandomDice* dice) const
	{
		if (minDamage == maxDamage)
			return minDamage;
		return dice->roll(minDamage, maxDamage);
	}

	int get_average_damage() const { return (minDamage + maxDamage) / 2; }

	// Modification operations
	DamageInfo& add_bonus(int bonus)
	{
		minDamage += bonus;
		maxDamage += bonus;
		dice.bonus += bonus;
		if (bonus > 0)
		{
			displayRoll += std::format("+{}", bonus);
		}
		else if (bonus < 0)
		{
			displayRoll += std::format("{}", bonus); // Already has minus sign
		}
		return *this;
	}

	// Create enhanced version with bonus (non-mutating)
	DamageInfo with_enhancement(int damage_bonus) const
	{
		DamageInfo enhanced = *this;
		enhanced.add_bonus(damage_bonus);
		return enhanced;
	}

	DamageInfo& multiply_damage(float multiplier)
	{
		minDamage = static_cast<int>(minDamage * multiplier);
		maxDamage = static_cast<int>(maxDamage * multiplier);
		// Don't modify display roll for multipliers - too complex
		return *this;
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
