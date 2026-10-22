#pragma once

// file: Spellcasting.h
//
// What a creature can cast and what it has left. Three parts, each one a rule of the book:
// - the memorized spells, which a wizard or a priest fills from its slots and spends one copy
//   at a time (PHB, PDF page 165), the same list for the player and for a monster;
// - the training a monster record states, its caster class and level, because a monster has
//   no class and its Hit Dice are not its caster level; the player's are derived instead;
// - the innate powers a monster uses as spells, each at will or once a day, at a caster level
//   equal to its Hit Dice (DMG, PDF page 718).
//
// Usage:
//
//   Spellcasting lich{};
//   lich.memorizedSpells = { "magic_missile", "magic_missile", "fireball" };
//   spend_memorized(lich, "magic_missile");               // -> { "magic_missile", "fireball" }
//
//   InnatePower breath{ "fireball", PowerFrequency::ONCE_PER_DAY, 0 };
//   record_power_use(breath, 1200);                       // ready again a day of time units later
//   is_power_ready(breath, 1200);                         // -> false
//
//   parse_power_frequency("at_will");                     // -> PowerFrequency::AT_WILL
//   parse_power_frequency("twice_a_week");                // throws std::runtime_error
//   parse_caster_class("wizard");                         // -> CasterClass::WIZARD
//   parse_spellcasting(encode_spellcasting(lich));        // the same three parts back

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "SpellRegistry.h"

// How often an innate power can be used.
enum class PowerFrequency
{
	// As often as the creature acts, one power a round.
	AT_WILL,
	// Once, then not again until a day has passed on the clock.
	ONCE_PER_DAY
};

// The name a record and a save write a frequency as.
//
// Example:
//   encode_power_frequency(PowerFrequency::ONCE_PER_DAY);   // -> "once_per_day"
[[nodiscard]] std::string_view encode_power_frequency(PowerFrequency frequency);

// The frequency a name stands for. Throws std::runtime_error naming what it read.
//
// Example:
//   parse_power_frequency("at_will");    // -> PowerFrequency::AT_WILL
//   parse_power_frequency("weekly");     // throws std::runtime_error
[[nodiscard]] PowerFrequency parse_power_frequency(std::string_view name);

// The name a record writes a caster class as. Refuses CasterClass::NONE, which no record names.
//
// Example:
//   encode_caster_class(CasterClass::CLERIC);   // -> "cleric"
[[nodiscard]] std::string_view encode_caster_class(CasterClass casterClass);

// The caster class a record names: "wizard" or "cleric". Throws std::runtime_error on
// anything else, "none" included, since a record with no casting writes null instead.
//
// Example:
//   parse_caster_class("cleric");   // -> CasterClass::CLERIC
//   parse_caster_class("none");     // throws std::runtime_error
[[nodiscard]] CasterClass parse_caster_class(std::string_view name);

// The caster class and level a monster record states.
struct SpellcasterTraining
{
	// Which progression table and which spell list it memorizes from.
	CasterClass casterClass{ CasterClass::NONE };
	// The level it memorizes and casts at.
	int casterLevel{ 0 };
};

// One spell a monster can use without memorizing it.
struct InnatePower
{
	// The spell's key in the spell registry.
	std::string spell{};
	// How often it can be used.
	PowerFrequency frequency{ PowerFrequency::AT_WILL };
	// The clock time, in time units, a once-a-day power is ready again at; 0 is ready.
	int readyAgainAt{ 0 };
};

// Everything a creature casts from.
struct Spellcasting
{
	// The spells memorized and not yet cast, one entry per copy.
	std::vector<std::string> memorizedSpells{};
	// A monster's stated class and level; empty for the player and for a monster that casts
	// only innate powers.
	std::optional<SpellcasterTraining> training{};
	// A monster's innate powers, in the order its record lists them.
	std::vector<InnatePower> innatePowers{};
};

// Whether the power can be used at clock time now. An at-will power always can.
//
// Example:
//   is_power_ready(InnatePower{ "fireball", PowerFrequency::AT_WILL, 0 }, 500);   // -> true
[[nodiscard]] bool is_power_ready(const InnatePower& power, int now);

// Records that the power was used at now: a once-a-day power is ready again a day of
// rounds later, and an at-will power is unchanged.
//
// Example:
//   record_power_use(oncePerDay, 120);   // oncePerDay.readyAgainAt -> 120 + 1440 * 120
void record_power_use(InnatePower& power, int now);

// Removes one memorized copy of the spell, since each memorization is one casting. Asserts
// the spell was memorized.
//
// Example:
//   spend_memorized(lich, "magic_missile");   // { mm, mm, fireball } -> { mm, fireball }
void spend_memorized(Spellcasting& spellcasting, std::string_view key);

// The save's form of all three parts, with each power's readyAgainAt.
//
// Example:
//   encode_spellcasting(Spellcasting{});
//   // -> { "memorizedSpells": [], "training": null, "innatePowers": [] }
[[nodiscard]] nlohmann::json encode_spellcasting(const Spellcasting& spellcasting);

// The inverse of encode_spellcasting. Throws when a field is missing or a name is unknown.
//
// Example:
//   parse_spellcasting(nlohmann::json{ { "memorizedSpells", nlohmann::json::array() } });
//   // throws nlohmann::json::out_of_range, no "training"
[[nodiscard]] Spellcasting parse_spellcasting(const nlohmann::json& record);

// end of file: Spellcasting.h
