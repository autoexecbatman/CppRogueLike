#pragma once

#include <array>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace CppRL
{

// file: SpellRegistry.h
//
// Every spell the game knows: the builtins, whose effects are compiled in and whose
// name, level, class and description are data, and the custom spells the spell editor
// adds. Game owns the registry the game plays with and hands it out as
// ctx.spellRegistry; SpellSystem casts what it holds. Each registry is its own value,
// so a test builds one and an edit to it reaches nothing else.
//
// Usage:
//
//   SpellRegistry spells;
//   spells.get_by_key("fireball").effect_type; // -> FIREBALL, before any load
//   spells.load("data/content/spells.json"); // throws if missing or a builtin has no record
//   spells.get_by_key("bless").level; // -> 1
//   spells.add_custom(SpellDefinition{ .name = "Registry Spark" }); // -> "registry_spark"
//   spells.get_by_key("no_such_spell"); // throws std::out_of_range
//   spells.save("data/content/spells.json");

// The spell list a caster draws from, which SpellSystem derives from the caster's CreatureClass
enum class CasterClass
{
	CLERIC,
	WIZARD,
	NONE
};

// Which casters may learn a spell.
enum class SpellClass
{
	CLERIC,
	WIZARD,
	BOTH
};

// Which builtin effect implementation a spell uses.
// Builtins have this set at initialisation. Custom spells pick one in SpellEditor.
enum class SpellEffectType
{
	CURE_LIGHT_WOUNDS,
	BLESS,
	SANCTUARY,
	PROTECTION_FROM_EVIL,
	HOLD_PERSON,
	SILENCE,
	MAGIC_MISSILE,
	SHIELD,
	SLEEP,
	INVISIBILITY,
	WEB,
	FIREBALL,
	TELEPORT,
	KNOCK,
	NONE
};

// One spell effect, a row of SPELL_EFFECT_TABLE.
struct SpellEffectRecord
{
	// The effect this row is for.
	SpellEffectType effect{ SpellEffectType::NONE };
	// Its key in spells.json, and the key of the builtin spell that casts it.
	std::string_view key{};
	// What the spell editor shows for it.
	std::string_view name{};
};

// How many effects there are, NONE included, as a size.
inline constexpr std::size_t SPELL_EFFECT_COUNT =
	static_cast<std::size_t>(SpellEffectType::NONE) + 1;

// Every SpellEffectType, one row each in the order the enum declares them.
// This is the only list of the effects; everything that names them reads it.
inline constexpr std::array<SpellEffectRecord, SPELL_EFFECT_COUNT>
	SPELL_EFFECT_TABLE{ {
		{
			SpellEffectType::CURE_LIGHT_WOUNDS,
			"cure_light_wounds",
			"Cure Light Wounds",
		},
		{ SpellEffectType::BLESS, "bless", "Bless" },
		{ SpellEffectType::SANCTUARY, "sanctuary", "Sanctuary" },
		{
			SpellEffectType::PROTECTION_FROM_EVIL,
			"protection_from_evil",
			"Protection From Evil",
		},
		{ SpellEffectType::HOLD_PERSON, "hold_person", "Hold Person" },
		{ SpellEffectType::SILENCE, "silence", "Silence" },
		{ SpellEffectType::MAGIC_MISSILE, "magic_missile", "Magic Missile" },
		{ SpellEffectType::SHIELD, "shield", "Shield" },
		{ SpellEffectType::SLEEP, "sleep", "Sleep" },
		{ SpellEffectType::INVISIBILITY, "invisibility", "Invisibility" },
		{ SpellEffectType::WEB, "web", "Web" },
		{ SpellEffectType::FIREBALL, "fireball", "Fireball" },
		{ SpellEffectType::TELEPORT, "teleport", "Teleport" },
		{ SpellEffectType::KNOCK, "knock", "Knock" },
		{ SpellEffectType::NONE, "none", "None" },
	} };

// Whether row n of the table holds effect n with a key and a name. An effect
// added to the enum without a row leaves a defaulted row, which fails this.
[[nodiscard]] constexpr bool is_spell_effect_table_in_enum_order()
{
	for (std::size_t row = 0; row < SPELL_EFFECT_TABLE.size(); ++row)
	{
		if (SPELL_EFFECT_TABLE[row].effect !=
				static_cast<SpellEffectType>(row) ||
			SPELL_EFFECT_TABLE[row].key.empty() ||
			SPELL_EFFECT_TABLE[row].name.empty())
		{
			return false;
		}
	}
	return true;
}

static_assert(
	is_spell_effect_table_in_enum_order(),
	"SPELL_EFFECT_TABLE needs one row per SpellEffectType, in enum order");

// The table's row for an effect.
//
// Example:
//   spell_effect_record(SpellEffectType::KNOCK).name;   // -> "Knock"
[[nodiscard]] constexpr const SpellEffectRecord& spell_effect_record(
	SpellEffectType effect)
{
	return SPELL_EFFECT_TABLE.at(static_cast<std::size_t>(effect));
}

// One spell as the menus show it and casting dispatches it.
struct SpellDefinition
{
	// The name every menu shows.
	std::string name{};
	// The spell level a caster needs a slot of to memorise it.
	int level{};
	// Which casters may learn it.
	SpellClass spellClass{};
	// A one-line summary, edited in the spell editor.
	std::string description{};
	// The effect cast_spell_by_key runs.
	SpellEffectType effect_type{ SpellEffectType::NONE };
};

// The builtin and custom spells, by key; see the file comment above for use.
class SpellRegistry
{
private:
	// Every builtin by key, holding its compiled-in effect from construction.
	std::map<std::string, SpellDefinition, std::less<>> builtinSpells{};
	// User-created spells by key.
	std::map<std::string, SpellDefinition, std::less<>> customSpells{};

public:
	// Holds every builtin with its effect and no data, and no custom spells.
	SpellRegistry();

	// Reads spell data, replacing every builtin's data and every custom spell. A file
	// missing a builtin's record, or naming a class or effect the game does not know,
	// throws.
	void load(std::string_view path);

	// Writes every builtin without its effect, which is compiled in, then every
	// custom spell with it.
	void save(std::string_view path) const;

	// Builtin keys in their fixed order, then custom keys in key order.
	[[nodiscard]] std::vector<std::string> get_all_keys() const;

	// Throws std::out_of_range if key is unknown.
	[[nodiscard]] const SpellDefinition& get_by_key(std::string_view key) const;

	// Replaces a builtin or custom spell. Throws std::out_of_range if key is unknown.
	// A builtin keeps its effect: the definition must carry the one it has.
	void set_by_key(std::string_view key, const SpellDefinition& definition);

	// Adds a user-created spell under a key made from its name, numbered past any key
	// already taken, and returns that key.
	[[nodiscard]] std::string add_custom(SpellDefinition definition);

	// Removes a user-created spell. Throws std::invalid_argument for a builtin and
	// std::out_of_range for an unknown key.
	void remove_custom(std::string_view key);

	// Whether a key names a builtin, whose effect is compiled in and cannot be removed.
	[[nodiscard]] bool is_builtin_key(std::string_view key) const;

	// Every spell of the caster's class or of both classes, at or below a spell level:
	// builtins in their fixed order, then custom spells.
	[[nodiscard]] std::vector<std::string> get_available_spells(
		CasterClass casterClass,
		int maxSpellLevel) const;
};

} // namespace CppRL
