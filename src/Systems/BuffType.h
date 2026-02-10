#pragma once

// Unified buff system - single source of truth for all timed effects
enum class BuffType
{
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
};

struct Buff
{
	BuffType type{ BuffType::INVISIBILITY };
	int value{ 0 };           // Bonus amount (0 for binary buffs like invisibility)
	int turnsRemaining{ 0 };
	bool is_set_effect{ false };  // AD&D 2e: true = SET stat to value (potions), false = ADD value (spells/items)
	// Note: Modifier stack pattern - no originalStat needed, effective values calculated on the fly
};
