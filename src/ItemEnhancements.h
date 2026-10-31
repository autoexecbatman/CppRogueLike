#pragma once

#include <algorithm>
#include <array>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "DamageInfo.h"
#include "ItemIdentification.h"

namespace CppRL
{

class RandomDice;

// Whether a random draw may hand back one of the magical prefixes - FLAMING,
// FROST, SHOCK and MAGICAL. Every generator takes it, because which pool a
// draw uses depends on the item and the choice belongs to the caller.
enum class MagicalPrefixes
{
	ALLOWED,
	EXCLUDED,
};

enum class PrefixType
{
	NONE,
	// Weapon prefixes
	SHARP,
	KEEN,
	MASTERWORK,
	BLESSED,
	FLAMING,
	FROST,
	SHOCK,
	// Armor prefixes
	REINFORCED,
	STUDDED,
	ELVEN,
	DWARVEN,
	MAGICAL,
	// Universal prefixes
	CURSED,
	ANCIENT,
	RUSTED,
	CRACKED,
	COUNT
};

enum class SuffixType
{
	NONE,
	// Combat suffixes
	OF_SLAYING,
	OF_ACCURACY,
	OF_PROTECTION,
	OF_POWER,
	// Resistance suffixes
	OF_FIRE_RESISTANCE,
	OF_COLD_RESISTANCE,
	OF_LIGHTNING_RESISTANCE,
	OF_POISON_RESISTANCE,
	// Special suffixes
	OF_SPEED,
	OF_STEALTH,
	OF_MAGIC,
	OF_HEALTH,
	OF_THE_EAGLE,
	OF_THE_BEAR,
	OF_THE_OWL,
	// Negative suffixes
	OF_WEAKNESS,
	OF_SLOWNESS,
	OF_BRITTLENESS,
	COUNT
};

// Whether an enhancement makes its item magical.
enum class EnhancementMagic
{
	MUNDANE,
	MAGICAL,
};

// How an enhancement changes an item's value percentage: adding to it, or
// replacing it outright.
enum class ValueChange
{
	ADD,
	SET,
};

// What one prefix or suffix does to an enhancement. The bonuses add. A
// resistance or a blessing is set only where the row gives one: nullopt
// leaves the enhancement's own value, where a value replaces it. A
// resistance is in percent, and any value above 0 counts as one ring's worth.
// The prefix's row is applied before the suffix's, so a suffix that sets a
// field overrides its prefix.
struct EnhancementEffect
{
	// Flat points on the weapon's damage dice: 2 makes 1d8 into 1d8+2.
	int damage{ 0 };
	// Added to the attack roll; a cursed weapon takes -2 instead.
	int toHit{ 0 };
	// Shown in the item's stats; nothing adds it to armour class.
	int ac{ 0 };
	// Added to the wearer's Strength while the item is worn.
	int strength{ 0 };
	// Added to the wearer's Dexterity while the item is worn.
	int dexterity{ 0 };
	// Saved with the item; nothing applies it.
	int intelligence{ 0 };
	// Saved with the item; nothing applies it.
	int hp{ 0 };
	// Saved with the item; nothing applies it.
	int mana{ 0 };
	// Saved with the item; nothing applies it.
	int speed{ 0 };
	// Saved with the item; nothing applies it.
	int stealth{ 0 };
	// Resists fire damage.
	std::optional<int> fireResistance{};
	// Resists cold damage.
	std::optional<int> coldResistance{};
	// Resists lightning damage.
	std::optional<int> lightningResistance{};
	// Resists poison damage.
	std::optional<int> poisonResistance{};
	// CURSED binds the item to its wearer until Remove Curse breaks it.
	std::optional<BlessingStatus> blessing{};
	// MAGICAL marks the item magical and is what EXCLUDED strips.
	EnhancementMagic magic{ EnhancementMagic::MUNDANE };
	// ADD adds value to the price percentage, which starts at 100; SET sets it.
	ValueChange valueChange{ ValueChange::ADD };
	// Percentage points of the item's base price.
	int value{ 0 };
};

// One prefix: the word an item's name starts with, and what it does.
struct PrefixRecord
{
	// The prefix this row is for.
	PrefixType type{ PrefixType::NONE };
	// What a save writes for it.
	std::string_view key{};
	// The word shown before an item's name; empty for NONE.
	std::string_view name{};
	// What the prefix does to an enhancement.
	EnhancementEffect effect{};
};

// One suffix: the words an item's name ends with, and what it does.
struct SuffixRecord
{
	// The suffix this row is for.
	SuffixType type{ SuffixType::NONE };
	// What a save writes for it.
	std::string_view key{};
	// The words shown after an item's name; empty for NONE.
	std::string_view name{};
	// What the suffix does to an enhancement.
	EnhancementEffect effect{};
};

// Rows in PREFIX_TABLE: one per PrefixType, NONE included.
inline constexpr std::size_t PREFIX_COUNT =
	static_cast<std::size_t>(PrefixType::COUNT);
// Rows in SUFFIX_TABLE: one per SuffixType, NONE included.
inline constexpr std::size_t SUFFIX_COUNT =
	static_cast<std::size_t>(SuffixType::COUNT);

// Every prefix, one row each, in enum order.
inline constexpr std::array<PrefixRecord, PREFIX_COUNT> PREFIX_TABLE{ {
	PrefixRecord{
		PrefixType::NONE,
		"none",
		"",
		EnhancementEffect{},
	},
	PrefixRecord{
		PrefixType::SHARP,
		"sharp",
		"Sharp",
		EnhancementEffect{
			.damage = 1,
			.value = 50,
		},
	},
	PrefixRecord{
		PrefixType::KEEN,
		"keen",
		"Keen",
		EnhancementEffect{
			.damage = 2,
			.value = 100,
		},
	},
	PrefixRecord{
		PrefixType::MASTERWORK,
		"masterwork",
		"Masterwork",
		EnhancementEffect{
			.toHit = 1,
			.value = 75,
		},
	},
	PrefixRecord{
		PrefixType::BLESSED,
		"blessed",
		"Blessed",
		EnhancementEffect{
			.damage = 1,
			.toHit = 1,
			.blessing = BlessingStatus::BLESSED,
			.value = 150,
		},
	},
	PrefixRecord{
		PrefixType::FLAMING,
		"flaming",
		"Flaming",
		EnhancementEffect{
			.damage = 2,
			.fireResistance = 25,
			.magic = EnhancementMagic::MAGICAL,
			.value = 200,
		},
	},
	PrefixRecord{
		PrefixType::FROST,
		"frost",
		"Frost",
		EnhancementEffect{
			.damage = 2,
			.coldResistance = 25,
			.magic = EnhancementMagic::MAGICAL,
			.value = 200,
		},
	},
	PrefixRecord{
		PrefixType::SHOCK,
		"shock",
		"Shock",
		EnhancementEffect{
			.damage = 2,
			.lightningResistance = 25,
			.magic = EnhancementMagic::MAGICAL,
			.value = 200,
		},
	},
	PrefixRecord{
		PrefixType::REINFORCED,
		"reinforced",
		"Reinforced",
		EnhancementEffect{
			.ac = 1,
			.value = 60,
		},
	},
	PrefixRecord{
		PrefixType::STUDDED,
		"studded",
		"Studded",
		EnhancementEffect{
			.ac = 1,
			.value = 40,
		},
	},
	PrefixRecord{
		PrefixType::ELVEN,
		"elven",
		"Elven",
		EnhancementEffect{
			.ac = 2,
			.dexterity = 1,
			.value = 300,
		},
	},
	PrefixRecord{
		PrefixType::DWARVEN,
		"dwarven",
		"Dwarven",
		EnhancementEffect{
			.ac = 3,
			.strength = 1,
			.value = 400,
		},
	},
	PrefixRecord{
		PrefixType::MAGICAL,
		"magical",
		"Magical",
		EnhancementEffect{
			.ac = 1,
			.magic = EnhancementMagic::MAGICAL,
			.value = 250,
		},
	},
	PrefixRecord{
		PrefixType::CURSED,
		"cursed",
		"Cursed",
		EnhancementEffect{
			.blessing = BlessingStatus::CURSED,
			.valueChange = ValueChange::SET,
			.value = 10,
		},
	},
	PrefixRecord{
		PrefixType::ANCIENT,
		"ancient",
		"Ancient",
		EnhancementEffect{
			.value = 500,
		},
	},
	PrefixRecord{
		PrefixType::RUSTED,
		"rusted",
		"Rusted",
		EnhancementEffect{
			.damage = -1,
			.ac = -1,
			.valueChange = ValueChange::SET,
			.value = 30,
		},
	},
	PrefixRecord{
		PrefixType::CRACKED,
		"cracked",
		"Cracked",
		EnhancementEffect{
			.valueChange = ValueChange::SET,
			.value = 50,
		},
	},
} };

// Every suffix, one row each, in enum order.
inline constexpr std::array<SuffixRecord, SUFFIX_COUNT> SUFFIX_TABLE{ {
	SuffixRecord{
		SuffixType::NONE,
		"none",
		"",
		EnhancementEffect{},
	},
	SuffixRecord{
		SuffixType::OF_SLAYING,
		"of_slaying",
		"of Slaying",
		EnhancementEffect{
			.damage = 3,
			.value = 300,
		},
	},
	SuffixRecord{
		SuffixType::OF_ACCURACY,
		"of_accuracy",
		"of Accuracy",
		EnhancementEffect{
			.toHit = 2,
			.value = 200,
		},
	},
	SuffixRecord{
		SuffixType::OF_PROTECTION,
		"of_protection",
		"of Protection",
		EnhancementEffect{
			.ac = 2,
			.value = 250,
		},
	},
	SuffixRecord{
		SuffixType::OF_POWER,
		"of_power",
		"of Power",
		EnhancementEffect{
			.strength = 1,
			.dexterity = 1,
			.intelligence = 1,
			.value = 400,
		},
	},
	SuffixRecord{
		SuffixType::OF_FIRE_RESISTANCE,
		"of_fire_resistance",
		"of Fire Resistance",
		EnhancementEffect{
			.fireResistance = 50,
			.value = 150,
		},
	},
	SuffixRecord{
		SuffixType::OF_COLD_RESISTANCE,
		"of_cold_resistance",
		"of Cold Resistance",
		EnhancementEffect{
			.coldResistance = 50,
			.value = 150,
		},
	},
	SuffixRecord{
		SuffixType::OF_LIGHTNING_RESISTANCE,
		"of_lightning_resistance",
		"of Lightning Resistance",
		EnhancementEffect{
			.lightningResistance = 50,
			.value = 150,
		},
	},
	SuffixRecord{
		SuffixType::OF_POISON_RESISTANCE,
		"of_poison_resistance",
		"of Poison Resistance",
		EnhancementEffect{
			.poisonResistance = 50,
			.value = 150,
		},
	},
	SuffixRecord{
		SuffixType::OF_SPEED,
		"of_speed",
		"of Speed",
		EnhancementEffect{
			.speed = 2,
			.value = 200,
		},
	},
	SuffixRecord{
		SuffixType::OF_STEALTH,
		"of_stealth",
		"of Stealth",
		EnhancementEffect{
			.stealth = 3,
			.value = 175,
		},
	},
	SuffixRecord{
		SuffixType::OF_MAGIC,
		"of_magic",
		"of Magic",
		EnhancementEffect{
			.intelligence = 1,
			.mana = 20,
			.value = 225,
		},
	},
	SuffixRecord{
		SuffixType::OF_HEALTH,
		"of_health",
		"of Health",
		EnhancementEffect{
			.hp = 15,
			.value = 200,
		},
	},
	SuffixRecord{
		SuffixType::OF_THE_EAGLE,
		"of_the_eagle",
		"of the Eagle",
		EnhancementEffect{
			.dexterity = 2,
			.value = 150,
		},
	},
	SuffixRecord{
		SuffixType::OF_THE_BEAR,
		"of_the_bear",
		"of the Bear",
		EnhancementEffect{
			.strength = 2,
			.value = 150,
		},
	},
	SuffixRecord{
		SuffixType::OF_THE_OWL,
		"of_the_owl",
		"of the Owl",
		EnhancementEffect{
			.intelligence = 2,
			.value = 150,
		},
	},
	SuffixRecord{
		SuffixType::OF_WEAKNESS,
		"of_weakness",
		"of Weakness",
		EnhancementEffect{
			.strength = -1,
			.blessing = BlessingStatus::CURSED,
			.valueChange = ValueChange::SET,
			.value = 20,
		},
	},
	SuffixRecord{
		SuffixType::OF_SLOWNESS,
		"of_slowness",
		"of Slowness",
		EnhancementEffect{
			.speed = -2,
			.blessing = BlessingStatus::CURSED,
			.valueChange = ValueChange::SET,
			.value = 15,
		},
	},
	SuffixRecord{
		SuffixType::OF_BRITTLENESS,
		"of_brittleness",
		"of Brittleness",
		EnhancementEffect{
			.blessing = BlessingStatus::CURSED,
			.valueChange = ValueChange::SET,
			.value = 25,
		},
	},
} };

// Whether a table has row i naming enumerator i, with a key on every row and a
// name on every row but NONE's.
//
// Example:
//   is_enhancement_table_in_enum_order(PREFIX_TABLE);   // -> true
//   // two rows swapped -> false, and the static_assert fails
template <typename Record, std::size_t Count>
[[nodiscard]] consteval bool is_enhancement_table_in_enum_order(
	const std::array<Record, Count>& table)
{
	using Type = decltype(Record::type);
	for (std::size_t row = 0; row < Count; ++row)
	{
		const bool isInOrder = table[row].type == static_cast<Type>(row);
		const bool isKeyed = !table[row].key.empty();
		const bool isNamed = (row == 0) == table[row].name.empty();
		if (!isInOrder || !isKeyed || !isNamed)
		{
			return false;
		}
	}
	return true;
}

static_assert(
	is_enhancement_table_in_enum_order(PREFIX_TABLE),
	"PREFIX_TABLE needs one row per PrefixType, in enum order");
static_assert(
	is_enhancement_table_in_enum_order(SUFFIX_TABLE),
	"SUFFIX_TABLE needs one row per SuffixType, in enum order");

// The table's row for a prefix or a suffix. COUNT has no row.
//
// Example:
//   prefix_record(PrefixType::KEEN).name;   // -> "Keen"
//   suffix_record(SuffixType::OF_SPEED).effect.speed;   // -> 2
[[nodiscard]] constexpr const PrefixRecord& prefix_record(PrefixType prefix)
{
	return PREFIX_TABLE.at(static_cast<std::size_t>(prefix));
}

// The suffix half of prefix_record above.
[[nodiscard]] constexpr const SuffixRecord& suffix_record(SuffixType suffix)
{
	return SUFFIX_TABLE.at(static_cast<std::size_t>(suffix));
}

// The key a prefix or a suffix is saved under: its enumerator in snake_case.
//
// Example:
//   encode_prefix(PrefixType::KEEN);             // -> "keen"
//   encode_suffix(SuffixType::OF_THE_OWL);       // -> "of_the_owl"
[[nodiscard]] constexpr std::string_view encode_prefix(PrefixType prefix)
{
	return prefix_record(prefix).key;
}

// The suffix half of encode_prefix above.
[[nodiscard]] constexpr std::string_view encode_suffix(SuffixType suffix)
{
	return suffix_record(suffix).key;
}

// The prefix or suffix a saved key names. Throws std::runtime_error for a key
// no row has.
//
// Example:
//   parse_prefix("keen");         // -> PrefixType::KEEN
//   parse_prefix("blunt");        // throws "unknown prefix 'blunt'"
//   parse_suffix("of_sandals");   // throws "unknown suffix 'of_sandals'"
[[nodiscard]] inline PrefixType parse_prefix(std::string_view key)
{
	const auto found = std::ranges::find(PREFIX_TABLE, key, &PrefixRecord::key);
	if (found == PREFIX_TABLE.end())
	{
		throw std::runtime_error(std::format("unknown prefix '{}'", key));
	}
	return found->type;
}

// The suffix half of parse_prefix above.
[[nodiscard]] inline SuffixType parse_suffix(std::string_view key)
{
	const auto found = std::ranges::find(SUFFIX_TABLE, key, &SuffixRecord::key);
	if (found == SUFFIX_TABLE.end())
	{
		throw std::runtime_error(std::format("unknown suffix '{}'", key));
	}
	return found->type;
}

struct ItemEnhancement
{
private:
	// Clears a magical prefix, before the effects are applied - they accumulate,
	// so a prefix cleared afterwards would leave its bonuses behind.
	void strip_magical_prefix();

	// Applies one row's effect: adds its bonuses, sets what it sets, and adds
	// to or replaces the value.
	void apply_effect(const EnhancementEffect& effect);

	static PrefixType get_random_weapon_prefix(RandomDice& dice);
	static PrefixType get_random_armor_prefix(RandomDice& dice);
	static PrefixType get_random_universal_prefix(RandomDice& dice);
	static SuffixType get_random_combat_suffix(RandomDice& dice);
	static SuffixType get_random_resistance_suffix(RandomDice& dice);
	static SuffixType get_random_special_suffix(RandomDice& dice);

public:
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

	// How strongly this enhancement resists the given type, in rings. The book
	// has no such suffix, so an enhancement of fire or cold resistance is read
	// as one ring's worth when its field is set. Zero for a type it says
	// nothing about.
	//
	// Example:
	//   enhancement.fireResistance = 50;
	//   enhancement.resistance_strength(DamageType::FIRE);   // -> 1
	//   enhancement.resistance_strength(DamageType::COLD);   // -> 0
	[[nodiscard]] int resistance_strength(DamageType damageType) const noexcept;

	// Special properties
	// Three-state: uncursed/blessed/cursed
	BlessingStatus blessing{ BlessingStatus::UNCURSED };
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

	// Apply enhancement effects
	void apply_enhancement_effects();

	// Enhancement generation
	static ItemEnhancement generate_random_enhancement(
		MagicalPrefixes magicalPrefixes,
		RandomDice& dice);
	static ItemEnhancement generate_weapon_enhancement(
		MagicalPrefixes magicalPrefixes,
		RandomDice& dice);
	static ItemEnhancement generate_armor_enhancement(
		MagicalPrefixes magicalPrefixes,
		RandomDice& dice);
};

} // namespace CppRL
