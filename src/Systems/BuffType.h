#pragma once

#include <format>
#include <stdexcept>
#include <string_view>

// Unified buff system - single source of truth for all timed effects
// AD&D 2e, Player's Handbook page 277: "all attacks made by evil (or evilly
// enchanted) creatures against the protected creature suffer -2 penalties to
// attack rolls".
inline constexpr int PROTECTION_FROM_EVIL_PENALTY = -2;

enum class BuffType
{
	NONE,
	INVISIBILITY,
	BLESS,
	SHIELD,
	STRENGTH,
	DEXTERITY,
	CONSTITUTION,
	INTELLIGENCE,
	WISDOM,
	CHARISMA,
	SPEED,
	FIRE_RESISTANCE,
	COLD_RESISTANCE,
	LIGHTNING_RESISTANCE,
	POISON_RESISTANCE,
	SLEEP,
	HOLD_PERSON,
	SANCTUARY,
	PROTECTION_FROM_EVIL,
	SILENCE,
	WEBBED,
};

// Maps a buff type to the string used in items.json and shown in the editor.
//
// Deliberately without a default case: adding a BuffType makes both this and
// every other switch over the enum fail to compile under -Wswitch, which is how
// PROTECTION_FROM_EVIL was caught silently encoding as "none".
//
// Example:
//   encode_buff_type(BuffType::BLESS);     // -> "bless"
//   encode_buff_type(BuffType::WEBBED);    // -> "webbed"
inline constexpr std::string_view encode_buff_type(BuffType buffType)
{
	switch (buffType)
	{
	case BuffType::NONE:
	{
		return "none";
	}
	case BuffType::INVISIBILITY:
	{
		return "invisibility";
	}
	case BuffType::BLESS:
	{
		return "bless";
	}
	case BuffType::SHIELD:
	{
		return "shield";
	}
	case BuffType::STRENGTH:
	{
		return "strength";
	}
	case BuffType::DEXTERITY:
	{
		return "dexterity";
	}
	case BuffType::CONSTITUTION:
	{
		return "constitution";
	}
	case BuffType::INTELLIGENCE:
	{
		return "intelligence";
	}
	case BuffType::WISDOM:
	{
		return "wisdom";
	}
	case BuffType::CHARISMA:
	{
		return "charisma";
	}
	case BuffType::SPEED:
	{
		return "speed";
	}
	case BuffType::FIRE_RESISTANCE:
	{
		return "fire_resistance";
	}
	case BuffType::COLD_RESISTANCE:
	{
		return "cold_resistance";
	}
	case BuffType::LIGHTNING_RESISTANCE:
	{
		return "lightning_resistance";
	}
	case BuffType::POISON_RESISTANCE:
	{
		return "poison_resistance";
	}
	case BuffType::SLEEP:
	{
		return "sleep";
	}
	case BuffType::HOLD_PERSON:
	{
		return "hold_person";
	}
	case BuffType::SANCTUARY:
	{
		return "sanctuary";
	}
	case BuffType::PROTECTION_FROM_EVIL:
	{
		return "protection_from_evil";
	}
	case BuffType::SILENCE:
	{
		return "silence";
	}
	case BuffType::WEBBED:
	{
		return "webbed";
	}
	}

	return "none";
}

struct Buff
{
	BuffType type{ BuffType::INVISIBILITY };
	int value{ 0 }; // Bonus amount (0 for binary buffs like invisibility)
	int turnsRemaining{ 0 };
	bool isSetEffect{ false }; // AD&D 2e: true = SET stat to value (potions), false = ADD value (spells/items)
	// Note: Modifier stack pattern - no originalStat needed, effective values calculated on the fly
};

inline BuffType parse_buff_type(std::string_view s)
{
	if (s == "none")
	{
		return BuffType::NONE;
	}
	if (s == "invisibility")
	{
		return BuffType::INVISIBILITY;
	}
	if (s == "bless")
	{
		return BuffType::BLESS;
	}
	if (s == "shield")
	{
		return BuffType::SHIELD;
	}
	if (s == "strength")
	{
		return BuffType::STRENGTH;
	}
	if (s == "dexterity")
	{
		return BuffType::DEXTERITY;
	}
	if (s == "constitution")
	{
		return BuffType::CONSTITUTION;
	}
	if (s == "intelligence")
	{
		return BuffType::INTELLIGENCE;
	}
	if (s == "wisdom")
	{
		return BuffType::WISDOM;
	}
	if (s == "charisma")
	{
		return BuffType::CHARISMA;
	}
	if (s == "speed")
	{
		return BuffType::SPEED;
	}
	if (s == "fire_resistance")
	{
		return BuffType::FIRE_RESISTANCE;
	}
	if (s == "cold_resistance")
	{
		return BuffType::COLD_RESISTANCE;
	}
	if (s == "lightning_resistance")
	{
		return BuffType::LIGHTNING_RESISTANCE;
	}
	if (s == "poison_resistance")
	{
		return BuffType::POISON_RESISTANCE;
	}
	if (s == "sleep")
	{
		return BuffType::SLEEP;
	}
	if (s == "hold_person")
	{
		return BuffType::HOLD_PERSON;
	}
	if (s == "sanctuary")
	{
		return BuffType::SANCTUARY;
	}
	if (s == "protection_from_evil")
	{
		return BuffType::PROTECTION_FROM_EVIL;
	}
	if (s == "silence")
	{
		return BuffType::SILENCE;
	}
	if (s == "webbed")
	{
		return BuffType::WEBBED;
	}

	throw std::runtime_error(std::format("unknown buff_type '{}'", s));
}
