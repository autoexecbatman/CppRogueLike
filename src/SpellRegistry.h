#pragma once

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

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

// Mirror of Player::PlayerClassState to avoid circular dependency
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
	[[nodiscard]] std::vector<std::string> get_available_spells(CasterClass casterClass, int maxSpellLevel) const;
};
