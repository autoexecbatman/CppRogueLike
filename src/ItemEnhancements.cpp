#include <iterator>
#include <string>
#include <utility>

#include "DamageInfo.h"
#include "ItemEnhancements.h"
#include "RandomDice.h"

namespace CppRL
{

std::string ItemEnhancement::get_prefix_name() const
{
	return std::string{ prefix_record(prefix).name };
}

std::string ItemEnhancement::get_suffix_name() const
{
	return std::string{ suffix_record(suffix).name };
}

int ItemEnhancement::resistance_strength(DamageType damageType) const noexcept
{
	// Any non-zero field is one ring's worth: the magnitude is not the book's unit.
	switch (damageType)
	{
	case DamageType::FIRE:
	{
		return fireResistance > 0 ? 1 : 0;
	}
	case DamageType::COLD:
	{
		return coldResistance > 0 ? 1 : 0;
	}
	case DamageType::LIGHTNING:
	{
		return lightningResistance > 0 ? 1 : 0;
	}
	case DamageType::POISON:
	{
		return poisonResistance > 0 ? 1 : 0;
	}
	case DamageType::PHYSICAL:
	case DamageType::ACID:
	case DamageType::MAGIC:
	{
		return 0;
	}
	}
	// Every damage type returned above; a new one fails to compile under -Wswitch
	// rather than falling through to a quiet zero.
	std::unreachable();
}

void ItemEnhancement::apply_effect(const EnhancementEffect& effect)
{
	damageBonus += effect.damage;
	toHitBonus += effect.toHit;
	acBonus += effect.ac;
	strengthBonus += effect.strength;
	dexterityBonus += effect.dexterity;
	intelligenceBonus += effect.intelligence;
	hpBonus += effect.hp;
	manaBonus += effect.mana;
	speedBonus += effect.speed;
	stealthBonus += effect.stealth;
	fireResistance = effect.fireResistance.value_or(fireResistance);
	coldResistance = effect.coldResistance.value_or(coldResistance);
	lightningResistance =
		effect.lightningResistance.value_or(lightningResistance);
	poisonResistance = effect.poisonResistance.value_or(poisonResistance);
	blessing = effect.blessing.value_or(blessing);
	if (effect.magic == EnhancementMagic::MAGICAL)
	{
		isMagical = true;
	}
	if (effect.valueChange == ValueChange::SET)
	{
		valueModifier = effect.value;
	}
	else
	{
		valueModifier += effect.value;
	}
}

void ItemEnhancement::apply_enhancement_effects()
{
	// The suffix goes second, so what it sets overrides the prefix.
	apply_effect(prefix_record(prefix).effect);
	apply_effect(suffix_record(suffix).effect);
}

// Random generation methods
ItemEnhancement ItemEnhancement::generate_random_enhancement(
	MagicalPrefixes magicalPrefixes,
	RandomDice& dice)
{
	ItemEnhancement enhancement;

	// 30% chance for prefix, 25% chance for suffix, 5% chance for both
	int roll = dice.roll(0, 99);

	if (roll < 30) // Prefix only
	{
		enhancement.prefix = get_random_universal_prefix(dice);
	}
	else if (roll < 55) // Suffix only
	{
		enhancement.suffix = get_random_special_suffix(dice);
	}
	else if (roll < 60) // Both prefix and suffix
	{
		enhancement.prefix = get_random_universal_prefix(dice);
		enhancement.suffix = get_random_special_suffix(dice);
	}
	// 40% chance for no enhancement

	if (magicalPrefixes == MagicalPrefixes::EXCLUDED)
	{
		enhancement.strip_magical_prefix();
	}

	enhancement.apply_enhancement_effects();
	return enhancement;
}

// Clears the prefix when its row makes the item magical, leaving any suffix
// alone. Every generator calls this before apply_enhancement_effects, whose
// bonuses accumulate, so a prefix cleared afterwards would leave its own behind.
//
// Example:
//   enhancement.prefix = PrefixType::FLAMING;
//   enhancement.strip_magical_prefix();   // prefix is now NONE
//   enhancement.prefix = PrefixType::SHARP;
//   enhancement.strip_magical_prefix();   // SHARP is mundane and stays
void ItemEnhancement::strip_magical_prefix()
{
	if (prefix_record(prefix).effect.magic == EnhancementMagic::MAGICAL)
	{
		prefix = PrefixType::NONE;
	}
}

ItemEnhancement ItemEnhancement::generate_weapon_enhancement(
	MagicalPrefixes magicalPrefixes,
	RandomDice& dice)
{
	ItemEnhancement enhancement;

	int roll = dice.roll(0, 99);
	if (roll < 40)
	{
		enhancement.prefix = get_random_weapon_prefix(dice);
	}
	if (roll >= 20 && roll < 60)
	{
		enhancement.suffix = get_random_combat_suffix(dice);
	}

	if (magicalPrefixes == MagicalPrefixes::EXCLUDED)
	{
		enhancement.strip_magical_prefix();
	}

	enhancement.apply_enhancement_effects();
	return enhancement;
}

ItemEnhancement ItemEnhancement::generate_armor_enhancement(
	MagicalPrefixes magicalPrefixes,
	RandomDice& dice)
{
	ItemEnhancement enhancement;

	int roll = dice.roll(0, 99);
	if (roll < 35)
	{
		enhancement.prefix = get_random_armor_prefix(dice);
	}
	if (roll >= 25 && roll < 55)
	{
		enhancement.suffix = get_random_resistance_suffix(dice);
	}

	if (magicalPrefixes == MagicalPrefixes::EXCLUDED)
	{
		enhancement.strip_magical_prefix();
	}

	enhancement.apply_enhancement_effects();
	return enhancement;
}

// Private helper methods
PrefixType ItemEnhancement::get_random_weapon_prefix(RandomDice& dice)
{
	static const PrefixType weapon_prefixes[] = {
		PrefixType::SHARP,
		PrefixType::KEEN,
		PrefixType::MASTERWORK,
		PrefixType::BLESSED,
		PrefixType::FLAMING,
		PrefixType::FROST,
		PrefixType::SHOCK,
		PrefixType::ANCIENT,
		PrefixType::CURSED,
	};

	return weapon_prefixes[dice
			.roll(0, static_cast<int>(std::size(weapon_prefixes)) - 1)];
}

PrefixType ItemEnhancement::get_random_armor_prefix(RandomDice& dice)
{
	static const PrefixType armor_prefixes[] = {
		PrefixType::REINFORCED,
		PrefixType::STUDDED,
		PrefixType::ELVEN,
		PrefixType::DWARVEN,
		PrefixType::MAGICAL,
		PrefixType::ANCIENT,
		PrefixType::CURSED,
		PrefixType::RUSTED,
	};

	return armor_prefixes[dice
			.roll(0, static_cast<int>(std::size(armor_prefixes)) - 1)];
}

PrefixType ItemEnhancement::get_random_universal_prefix(RandomDice& dice)
{
	static const PrefixType universal_prefixes[] = {
		PrefixType::BLESSED,
		PrefixType::CURSED,
		PrefixType::ANCIENT,
		PrefixType::MAGICAL,
		PrefixType::RUSTED,
		PrefixType::CRACKED,
	};

	return universal_prefixes[dice
			.roll(0, static_cast<int>(std::size(universal_prefixes)) - 1)];
}

SuffixType ItemEnhancement::get_random_combat_suffix(RandomDice& dice)
{
	static const SuffixType combat_suffixes[] = {
		SuffixType::OF_SLAYING,
		SuffixType::OF_ACCURACY,
		SuffixType::OF_PROTECTION,
		SuffixType::OF_POWER,
		SuffixType::OF_THE_BEAR,
		SuffixType::OF_THE_EAGLE,
	};

	return combat_suffixes[dice
			.roll(0, static_cast<int>(std::size(combat_suffixes)) - 1)];
}

SuffixType ItemEnhancement::get_random_resistance_suffix(RandomDice& dice)
{
	static const SuffixType resistance_suffixes[] = {
		SuffixType::OF_FIRE_RESISTANCE,
		SuffixType::OF_COLD_RESISTANCE,
		SuffixType::OF_LIGHTNING_RESISTANCE,
		SuffixType::OF_POISON_RESISTANCE
	};

	return resistance_suffixes[dice
			.roll(0, static_cast<int>(std::size(resistance_suffixes)) - 1)];
}

SuffixType ItemEnhancement::get_random_special_suffix(RandomDice& dice)
{
	static const SuffixType special_suffixes[] = {
		SuffixType::OF_SPEED,
		SuffixType::OF_STEALTH,
		SuffixType::OF_MAGIC,
		SuffixType::OF_HEALTH,
		SuffixType::OF_THE_OWL,
		SuffixType::OF_WEAKNESS,
		SuffixType::OF_SLOWNESS,
		SuffixType::OF_BRITTLENESS,
	};

	return special_suffixes[dice
			.roll(0, static_cast<int>(std::size(special_suffixes)) - 1)];
}

} // namespace CppRL
