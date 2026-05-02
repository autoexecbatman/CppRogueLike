#pragma once

#include <string>

#include "../../Items/ItemIdentification.h"

enum class PrefixType
{
	NONE,
	// Weapon prefixes
	SHARP, // +1 damage
	KEEN, // +2 damage
	MASTERWORK, // +1 to hit
	BLESSED, // +1 to hit and damage
	FLAMING, // Fire damage
	FROST, // Cold damage
	SHOCK, // Lightning damage
	// Armor prefixes
	REINFORCED, // +1 AC
	STUDDED, // +1 AC
	ELVEN, // +2 AC, lighter
	DWARVEN, // +3 AC, heavier
	MAGICAL, // +1 AC, magical
	// Universal prefixes
	CURSED, // Negative effects
	ANCIENT, // Increased value
	RUSTED, // Decreased effectiveness
	CRACKED // Reduced durability
};

enum class SuffixType
{
	NONE,
	// Combat suffixes
	OF_SLAYING, // +3 damage
	OF_ACCURACY, // +2 to hit
	OF_PROTECTION, // +2 AC
	OF_POWER, // +1 all stats
	// Resistance suffixes
	OF_FIRE_RESISTANCE,
	OF_COLD_RESISTANCE,
	OF_LIGHTNING_RESISTANCE,
	OF_POISON_RESISTANCE,
	// Special suffixes
	OF_SPEED, // Movement bonus
	OF_STEALTH, // Stealth bonus
	OF_MAGIC, // Mana bonus
	OF_HEALTH, // HP bonus
	OF_THE_EAGLE, // Dexterity bonus
	OF_THE_BEAR, // Strength bonus
	OF_THE_OWL, // Intelligence bonus
	// Negative suffixes
	OF_WEAKNESS, // Stat penalty
	OF_SLOWNESS, // Movement penalty
	OF_BRITTLENESS // Durability penalty
};

struct ItemEnhancement
{
	PrefixType prefix{ PrefixType::NONE };
	SuffixType suffix{ SuffixType::NONE };

	// Modifier values
	int damageBonus{ 0 };
	int toHitBonus{ 0 };
	int acBonus{ 0 };
	int strengthBonus{ 0 };
	int dexterityBonus{ 0 };
	int intelligenceBonus{ 0 };
	int hpBonus{ 0 };
	int manaBonus{ 0 };
	int speedBonus{ 0 };
	int stealthBonus{ 0 };

	// Resistances (0-100%)
	int fireResistance{ 0 };
	int coldResistance{ 0 };
	int lightningResistance{ 0 };
	int poisonResistance{ 0 };

	// Special properties
	BlessingStatus blessing{ BlessingStatus::UNCURSED }; // Three-state: uncursed/blessed/cursed
	bool isMagical{ false };

	// Numeric enhancement level for traditional "+X" display
	int enhancementLevel{ 0 };

	// Value modifier (percentage)
	int valueModifier{ 100 }; // 100% = no change

	// Weight in standardized units (roughly 1 lb per unit)
	int weight{ 0 };

	// Generate enhancement name
	std::string get_prefix_name() const;
	std::string get_suffix_name() const;
	std::string get_full_name(const std::string& base_name) const;

	// Apply enhancement effects
	void apply_enhancement_effects();

	// Enhancement generation
	static ItemEnhancement generate_random_enhancement(bool allowMagical);
	static ItemEnhancement generate_weapon_enhancement();
	static ItemEnhancement generate_armor_enhancement();

	// Rarity-based generation
	static ItemEnhancement generate_by_rarity(int rarity_level); // 1-5

private:
	static PrefixType get_random_weapon_prefix();
	static PrefixType get_random_armor_prefix();
	static PrefixType get_random_universal_prefix();
	static SuffixType get_random_combat_suffix();
	static SuffixType get_random_resistance_suffix();
	static SuffixType get_random_special_suffix();
};
