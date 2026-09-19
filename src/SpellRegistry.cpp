#include <array>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "Paths.h"
#include "SpellRegistry.h"

namespace
{
// A builtin spell's key and the effect compiled in for it.
struct BuiltinSpell
{
	std::string_view key{};
	SpellEffectType effect{ SpellEffectType::NONE };
};

// Every builtin, in the order get_all_keys lists them.
constexpr auto BUILTIN_SPELLS = std::to_array<BuiltinSpell>({
	{ "cure_light_wounds", SpellEffectType::CURE_LIGHT_WOUNDS },
	{ "bless", SpellEffectType::BLESS },
	{ "sanctuary", SpellEffectType::SANCTUARY },
	{ "protection_from_evil", SpellEffectType::PROTECTION_FROM_EVIL },
	{ "hold_person", SpellEffectType::HOLD_PERSON },
	{ "silence", SpellEffectType::SILENCE },
	{ "magic_missile", SpellEffectType::MAGIC_MISSILE },
	{ "shield", SpellEffectType::SHIELD },
	{ "sleep", SpellEffectType::SLEEP },
	{ "invisibility", SpellEffectType::INVISIBILITY },
	{ "web", SpellEffectType::WEB },
	{ "fireball", SpellEffectType::FIREBALL },
	{ "teleport", SpellEffectType::TELEPORT },
	{ "knock", SpellEffectType::KNOCK },
});

// Every builtin holding its effect and no data yet.
std::map<std::string, SpellDefinition, std::less<>> builtin_spells_with_effects()
{
	std::map<std::string, SpellDefinition, std::less<>> spells{};
	for (const BuiltinSpell& builtin : BUILTIN_SPELLS)
	{
		spells.emplace(std::string{ builtin.key }, SpellDefinition{ .effect_type = builtin.effect });
	}
	return spells;
}

SpellClass parse_class(std::string_view text)
{
	if (text == "cleric")
	{
		return SpellClass::CLERIC;
	}
	else if (text == "wizard")
	{
		return SpellClass::WIZARD;
	}
	else if (text == "both")
	{
		return SpellClass::BOTH;
	}

	throw std::runtime_error(std::format("SpellRegistry: unknown spell class '{}'", text));
}

std::string encode_class(SpellClass spellClass)
{
	if (spellClass == SpellClass::CLERIC)
	{
		return "cleric";
	}
	else if (spellClass == SpellClass::WIZARD)
	{
		return "wizard";
	}
	else
	{
		return "both";
	}
}

SpellEffectType parse_effect_type(std::string_view text)
{
	if (text == "cure_light_wounds")
	{
		return SpellEffectType::CURE_LIGHT_WOUNDS;
	}
	else if (text == "bless")
	{
		return SpellEffectType::BLESS;
	}
	else if (text == "protection_from_evil")
	{
		return SpellEffectType::PROTECTION_FROM_EVIL;
	}
	else if (text == "sanctuary")
	{
		return SpellEffectType::SANCTUARY;
	}
	else if (text == "hold_person")
	{
		return SpellEffectType::HOLD_PERSON;
	}
	else if (text == "silence")
	{
		return SpellEffectType::SILENCE;
	}
	else if (text == "magic_missile")
	{
		return SpellEffectType::MAGIC_MISSILE;
	}
	else if (text == "shield")
	{
		return SpellEffectType::SHIELD;
	}
	else if (text == "sleep")
	{
		return SpellEffectType::SLEEP;
	}
	else if (text == "invisibility")
	{
		return SpellEffectType::INVISIBILITY;
	}
	else if (text == "web")
	{
		return SpellEffectType::WEB;
	}
	else if (text == "fireball")
	{
		return SpellEffectType::FIREBALL;
	}
	else if (text == "teleport")
	{
		return SpellEffectType::TELEPORT;
	}
	else if (text == "knock")
	{
		return SpellEffectType::KNOCK;
	}
	else if (text == "none")
	{
		return SpellEffectType::NONE;
	}

	throw std::runtime_error(std::format("SpellRegistry: unknown spell effect '{}'", text));
}

std::string encode_effect_type(SpellEffectType effect)
{
	switch (effect)
	{
	case SpellEffectType::CURE_LIGHT_WOUNDS:
	{
		return "cure_light_wounds";
	}

	case SpellEffectType::BLESS:
	{
		return "bless";
	}

	case SpellEffectType::SANCTUARY:
	{
		return "sanctuary";
	}

	case SpellEffectType::PROTECTION_FROM_EVIL:
	{
		return "protection_from_evil";
	}

	case SpellEffectType::HOLD_PERSON:
	{
		return "hold_person";
	}

	case SpellEffectType::SILENCE:
	{
		return "silence";
	}

	case SpellEffectType::MAGIC_MISSILE:
	{
		return "magic_missile";
	}

	case SpellEffectType::SHIELD:
	{
		return "shield";
	}

	case SpellEffectType::SLEEP:
	{
		return "sleep";
	}

	case SpellEffectType::INVISIBILITY:
	{
		return "invisibility";
	}

	case SpellEffectType::WEB:
	{
		return "web";
	}

	case SpellEffectType::FIREBALL:
	{
		return "fireball";
	}

	case SpellEffectType::TELEPORT:
	{
		return "teleport";
	}

	case SpellEffectType::KNOCK:
	{
		return "knock";
	}

	default:
	{
		return "none";
	}
	}
}
} // namespace

SpellRegistry::SpellRegistry()
	: builtinSpells{ builtin_spells_with_effects() }
{
}

void SpellRegistry::load(std::string_view path)
{
	const auto resolved = Paths::resolve(path);
	std::ifstream file(resolved);
	if (!file.is_open())
	{
		throw std::runtime_error(
			std::format("SpellRegistry::load -- cannot open '{}'", resolved.string()));
	}

	const nlohmann::json root = nlohmann::json::parse(file);

	customSpells.clear();

	// Every builtin's name, level, class and description are data, the table holding
	// only its effect, so a file without one is refused.
	for (const BuiltinSpell& builtin : BUILTIN_SPELLS)
	{
		const std::string key{ builtin.key };
		if (!root.contains(key))
		{
			throw std::runtime_error(
				std::format("SpellRegistry::load -- '{}' has no record for builtin spell '{}'", resolved.string(), key));
		}
		const nlohmann::json& record = root.at(key);
		SpellDefinition& definition = builtinSpells.at(key);
		definition.name = record.at("name").get<std::string>();
		definition.level = record.at("level").get<int>();
		definition.spellClass = parse_class(record.at("class").get<std::string>());
		definition.description = record.at("description").get<std::string>();
	}

	// Every other key is a custom spell, which carries its effect as data.
	for (const auto& [key, record] : root.items())
	{
		if (is_builtin_key(key))
		{
			continue;
		}

		SpellDefinition definition;
		definition.name = record.at("name").get<std::string>();
		definition.level = record.at("level").get<int>();
		definition.spellClass = parse_class(record.at("class").get<std::string>());
		definition.description = record.at("description").get<std::string>();
		if (record.contains("effect"))
		{
			definition.effect_type = parse_effect_type(record.at("effect").get<std::string>());
		}
		customSpells[key] = std::move(definition);
	}
}

void SpellRegistry::save(std::string_view path) const
{
	const auto resolved = Paths::resolve(path);
	std::filesystem::create_directories(resolved.parent_path());

	nlohmann::json root = nlohmann::json::object();

	for (const BuiltinSpell& builtin : BUILTIN_SPELLS)
	{
		const std::string key{ builtin.key };
		const SpellDefinition& definition = builtinSpells.at(key);
		root[key] = nlohmann::json{
			{ "name", definition.name },
			{ "level", definition.level },
			{ "class", encode_class(definition.spellClass) },
			{ "description", definition.description }
		};
	}

	for (const auto& [key, definition] : customSpells)
	{
		root[key] = nlohmann::json{
			{ "name", definition.name },
			{ "level", definition.level },
			{ "class", encode_class(definition.spellClass) },
			{ "description", definition.description },
			{ "effect", encode_effect_type(definition.effect_type) }
		};
	}

	std::ofstream file(resolved);
	if (!file.is_open())
	{
		throw std::runtime_error(
			std::format("SpellRegistry::save -- cannot open '{}' for writing", resolved.string()));
	}
	file << root.dump(4);
	if (file.fail())
	{
		throw std::runtime_error(
			std::format("SpellRegistry::save -- write failed for '{}'", resolved.string()));
	}
}

std::vector<std::string> SpellRegistry::get_all_keys() const
{
	std::vector<std::string> keys;
	keys.reserve(BUILTIN_SPELLS.size() + customSpells.size());
	for (const BuiltinSpell& builtin : BUILTIN_SPELLS)
	{
		keys.push_back(std::string{ builtin.key });
	}
	for (const auto& [key, _] : customSpells)
	{
		keys.push_back(key);
	}
	return keys;
}

const SpellDefinition& SpellRegistry::get_by_key(std::string_view key) const
{
	if (builtinSpells.contains(key))
	{
		return builtinSpells.at(std::string{ key });
	}
	if (customSpells.contains(key))
	{
		return customSpells.at(std::string{ key });
	}

	throw std::out_of_range(std::format("SpellRegistry::get_by_key -- unknown key '{}'", key));
}

void SpellRegistry::set_by_key(std::string_view key, const SpellDefinition& definition)
{
	if (builtinSpells.contains(key))
	{
		builtinSpells.at(std::string{ key }) = definition;
		return;
	}
	if (customSpells.contains(key))
	{
		customSpells.at(std::string{ key }) = definition;
		return;
	}

	throw std::out_of_range(std::format("SpellRegistry::set_by_key -- unknown key '{}'", key));
}

std::string SpellRegistry::add_custom(SpellDefinition definition)
{
	auto normalize = [](std::string_view name) -> std::string
	{
		std::string key;
		for (const char character : name)
		{
			if (std::isspace(static_cast<unsigned char>(character)))
			{
				key += '_';
			}
			else
			{
				key += static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
			}
		}

		return key;
	};

	auto has_key = [this](const std::string& key) -> bool
	{
		return is_builtin_key(key) || customSpells.contains(key);
	};

	const std::string base = normalize(definition.name.empty() ? "new_spell" : definition.name);
	std::string key = base;
	if (has_key(key))
	{
		for (int suffix = 2;; ++suffix)
		{
			key = std::format("{}_{}", base, suffix);
			if (!has_key(key))
			{
				break;
			}
		}
	}
	customSpells[key] = std::move(definition);

	return key;
}

void SpellRegistry::remove_custom(std::string_view key)
{
	if (is_builtin_key(key))
	{
		throw std::invalid_argument(
			std::format("SpellRegistry::remove_custom -- '{}' is a built-in spell and cannot be removed", key));
	}
	if (!customSpells.erase(std::string{ key }))
	{
		throw std::out_of_range(
			std::format("SpellRegistry::remove_custom -- unknown key '{}'", key));
	}
}

bool SpellRegistry::is_builtin_key(std::string_view key) const
{
	return builtinSpells.contains(key);
}

std::vector<std::string> SpellRegistry::get_available_spells(CasterClass casterClass, int maxSpellLevel) const
{
	const SpellClass targetClass = (casterClass == CasterClass::CLERIC) ? SpellClass::CLERIC : SpellClass::WIZARD;

	auto is_available = [targetClass, maxSpellLevel](const SpellDefinition& definition) -> bool
	{
		return (definition.spellClass == targetClass || definition.spellClass == SpellClass::BOTH) && definition.level <= maxSpellLevel;
	};

	std::vector<std::string> available;
	for (const BuiltinSpell& builtin : BUILTIN_SPELLS)
	{
		if (is_available(builtinSpells.at(std::string{ builtin.key })))
		{
			available.push_back(std::string{ builtin.key });
		}
	}
	for (const auto& [key, definition] : customSpells)
	{
		if (is_available(definition))
		{
			available.push_back(key);
		}
	}

	return available;
}
