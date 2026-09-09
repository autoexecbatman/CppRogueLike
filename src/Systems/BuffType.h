#pragma once

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

struct Buff
{
	BuffType type{ BuffType::INVISIBILITY };
	int value{ 0 }; // Bonus amount (0 for binary buffs like invisibility)
	int turnsRemaining{ 0 };
	bool isSetEffect{ false }; // AD&D 2e: true = SET stat to value (potions), false = ADD value (spells/items)
	// Note: Modifier stack pattern - no originalStat needed, effective values calculated on the fly
};
