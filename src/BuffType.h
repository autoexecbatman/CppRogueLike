#pragma once

#include <algorithm>
#include <array>

#include <format>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "UniqueId.h"

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

// Every BuffType, in the order the enum declares them, so a cycle through the
// editor's field reaches all of them and adding one is a single edit here.
inline constexpr std::array<BuffType, 21> ALL_BUFF_TYPE = {
	BuffType::NONE,
	BuffType::INVISIBILITY,
	BuffType::BLESS,
	BuffType::SHIELD,
	BuffType::STRENGTH,
	BuffType::DEXTERITY,
	BuffType::CONSTITUTION,
	BuffType::INTELLIGENCE,
	BuffType::WISDOM,
	BuffType::CHARISMA,
	BuffType::SPEED,
	BuffType::FIRE_RESISTANCE,
	BuffType::COLD_RESISTANCE,
	BuffType::LIGHTNING_RESISTANCE,
	BuffType::POISON_RESISTANCE,
	BuffType::SLEEP,
	BuffType::HOLD_PERSON,
	BuffType::SANCTUARY,
	BuffType::PROTECTION_FROM_EVIL,
	BuffType::SILENCE,
	BuffType::WEBBED,
};

// The next BuffType in that order, wrapping at the end.
//
// Example:
//   next_buff_type(BuffType::NONE);   // -> BuffType::INVISIBILITY
[[nodiscard]] inline BuffType next_buff_type(BuffType value)
{
	const auto found = std::ranges::find(ALL_BUFF_TYPE, value);
	if (found == ALL_BUFF_TYPE.end() || found + 1 == ALL_BUFF_TYPE.end())
	{
		return ALL_BUFF_TYPE.front();
	}
	return *(found + 1);
}

// Maps a buff type to the string used in items.json and shown in the editor.
//
// Deliberately without a default case, so adding a BuffType warns here and in every
// other switch over the enum under -Wswitch - which is how PROTECTION_FROM_EVIL was
// caught silently encoding as "none". No build passes -Werror, so it warns rather
// than refusing; ALL_BUFF_TYPE below is what the editor and the tests read.
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

// One opponent's saving throw against a casting, kept so it is rolled once.
struct OpponentSave
{
	UniqueId::IdType opponent{};
	bool isMade{ false };
};

struct Buff
{
	BuffType type{ BuffType::INVISIBILITY };
	int value{ 0 }; // Bonus amount (0 for binary buffs like invisibility)
	int turnsRemaining{ 0 };
	bool isSetEffect{ false }; // AD&D 2e: true = SET stat to value (potions), false = ADD value (spells/items)
	// Note: Modifier stack pattern - no originalStat needed, effective values calculated on the fly

	// The saves opponents have rolled against this casting. Sanctuary is the one that
	// asks: each opponent saves once and the result holds while the casting lasts, so
	// the record lives and ends with it (PHB page 436).
	std::vector<OpponentSave> opponentSaves{};
};

inline BuffType parse_buff_type(std::string_view name)
{
	if (name == "none")
	{
		return BuffType::NONE;
	}
	if (name == "invisibility")
	{
		return BuffType::INVISIBILITY;
	}
	if (name == "bless")
	{
		return BuffType::BLESS;
	}
	if (name == "shield")
	{
		return BuffType::SHIELD;
	}
	if (name == "strength")
	{
		return BuffType::STRENGTH;
	}
	if (name == "dexterity")
	{
		return BuffType::DEXTERITY;
	}
	if (name == "constitution")
	{
		return BuffType::CONSTITUTION;
	}
	if (name == "intelligence")
	{
		return BuffType::INTELLIGENCE;
	}
	if (name == "wisdom")
	{
		return BuffType::WISDOM;
	}
	if (name == "charisma")
	{
		return BuffType::CHARISMA;
	}
	if (name == "speed")
	{
		return BuffType::SPEED;
	}
	if (name == "fire_resistance")
	{
		return BuffType::FIRE_RESISTANCE;
	}
	if (name == "cold_resistance")
	{
		return BuffType::COLD_RESISTANCE;
	}
	if (name == "lightning_resistance")
	{
		return BuffType::LIGHTNING_RESISTANCE;
	}
	if (name == "poison_resistance")
	{
		return BuffType::POISON_RESISTANCE;
	}
	if (name == "sleep")
	{
		return BuffType::SLEEP;
	}
	if (name == "hold_person")
	{
		return BuffType::HOLD_PERSON;
	}
	if (name == "sanctuary")
	{
		return BuffType::SANCTUARY;
	}
	if (name == "protection_from_evil")
	{
		return BuffType::PROTECTION_FROM_EVIL;
	}
	if (name == "silence")
	{
		return BuffType::SILENCE;
	}
	if (name == "webbed")
	{
		return BuffType::WEBBED;
	}

	throw std::runtime_error(std::format("unknown buff_type '{}'", name));
}
