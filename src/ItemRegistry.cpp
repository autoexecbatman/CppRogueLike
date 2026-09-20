#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "BuffType.h"
#include "ItemClassification.h"
#include "ItemRegistry.h"
#include "MagicalItemEffects.h"
#include "Paths.h"
#include "Pickable.h"
#include "TargetMode.h"
#include "Weapons.h"

namespace
{

// An item record as read: the strings its params' views will point into once stored.
struct ParsedItem
{
	std::string name{};
	std::string category{};
	ItemParams params{};
};

// A key from a display name: lower case, whitespace as underscores.
std::string normalize_key(std::string_view raw)
{
	std::string key;
	key.reserve(raw.size());
	for (const char character : raw)
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
}

// ---------------------------------------------------------------------------
// Enum parse helpers
// ---------------------------------------------------------------------------
ConsumableEffect parse_consumable_effect(std::string_view name)
{
	if (name == "none")
	{
		return ConsumableEffect::NONE;
	}
	if (name == "heal")
	{
		return ConsumableEffect::HEAL;
	}
	if (name == "add_buff")
	{
		return ConsumableEffect::ADD_BUFF;
	}
	if (name == "fail")
	{
		return ConsumableEffect::FAIL;
	}

	throw std::runtime_error(std::format("ItemRegistry: unknown consumable_effect '{}'", name));
}

// Which pool an enhanced spawn rule draws from. A rule naming anything else is a typo
// in enhanced_rules.json rather than a third pool, so it is refused by name instead of
// quietly becoming armour.
//
// Example:
//   parse_enhancement_category("weapon");   // -> EnhancedItemCategory::WEAPON
//   parse_enhancement_category("wepaon");
//   // throws: ItemRegistry: unknown enhancement_category 'wepaon'
EnhancedItemCategory parse_enhancement_category(std::string_view name)
{
	if (name == "weapon")
	{
		return EnhancedItemCategory::WEAPON;
	}
	if (name == "armor")
	{
		return EnhancedItemCategory::ARMOR;
	}

	throw std::runtime_error(std::format("ItemRegistry: unknown enhancement_category '{}'", name));
}

// ---------------------------------------------------------------------------
// Enum encode helpers
// ---------------------------------------------------------------------------
std::string_view encode_consumable_effect(ConsumableEffect consumableEffect)
{
	switch (consumableEffect)
	{

	case ConsumableEffect::NONE:
	{
		return "none";
	}

	case ConsumableEffect::HEAL:
	{
		return "heal";
	}

	case ConsumableEffect::ADD_BUFF:
	{
		return "add_buff";
	}

	case ConsumableEffect::FAIL:
	{
		return "fail";
	}
	}

	return "none";
}

// ---------------------------------------------------------------------------
// JSON encode / parse
// ---------------------------------------------------------------------------
nlohmann::json encode_item(const ItemParams& params)
{
	nlohmann::json record;
	record["name"] = std::string{ params.name };
	record["category"] = std::string{ params.category };
	record["color"] = params.color;
	record["itemClass"] = encode_item_class(params.itemClass);
	record["value"] = params.value;
	record["pickableType"] = encode_pickable_type(params.pickableType);
	record["baseWeight"] = params.baseWeight;
	record["weight"] = params.weight;
	record["levelMin"] = params.levelMin;
	record["levelMax"] = params.levelMax;
	record["levelScaling"] = params.levelScaling;
	record["consumableAmount"] = params.consumableAmount;
	record["range"] = params.range;
	record["damage"] = params.damage;
	record["confuseTurns"] = params.confuseTurns;
	record["duration"] = params.duration;
	record["effect"] = encode_magical_effect(params.effect);
	record["effectBonus"] = params.effectBonus;
	record["strBonus"] = params.strBonus;
	record["dexBonus"] = params.dexBonus;
	record["conBonus"] = params.conBonus;
	record["intBonus"] = params.intBonus;
	record["wisBonus"] = params.wisBonus;
	record["chaBonus"] = params.chaBonus;
	record["isSetMode"] = params.isSetMode;
	record["exceptionalStrength"] = params.exceptionalStrength;
	record["nutritionValue"] = params.nutritionValue;
	record["acBonus"] = params.acBonus;
	record["ranged"] = params.ranged;
	record["handRequirement"] = encode_hand_requirement(params.handRequirement);
	record["weaponSize"] = encode_weapon_size(params.weaponSize);
	record["strengthRating"] = params.strengthRating;
	record["consumableEffect"] = encode_consumable_effect(params.consumableEffect);
	record["consumableBuff"] = encode_buff_type(params.consumableBuffType);
	record["targetMode"] = encode_target_mode(params.targetMode);
	record["scrollAnimation"] = encode_scroll_animation(params.scrollAnimation);

	return record;
}

// Reads a field every item record carries. encode_item writes every field
// unconditionally, so a record missing one is a corrupted or hand-edited file rather
// than a shorter record - this names the item and the field instead of handing back a
// default nobody authored.
//
// Example:
//   required_field(record, "health_potion", "baseWeight");   // -> 50
//   required_field(record, "health_potion", "weight");
//   // throws: ItemRegistry::load -- item 'health_potion' is missing required field 'weight'
const nlohmann::json& required_field(
	const nlohmann::json& record,
	const std::string& itemKey,
	const std::string& field)
{
	const auto found = record.find(field);
	if (found == record.end())
	{
		throw std::runtime_error(std::format(
			"ItemRegistry::load -- item '{}' is missing required field '{}'",
			itemKey,
			field));
	}
	return *found;
}

ParsedItem parse_item(const std::string& key, const nlohmann::json& record)
{
	ParsedItem parsed;
	parsed.name = required_field(record, key, "name").get<std::string>();
	parsed.category = required_field(record, key, "category").get<std::string>();

	ItemParams& params = parsed.params;
	params.color = required_field(record, key, "color");
	params.itemClass = parse_item_class(required_field(record, key, "itemClass").get<std::string>());
	params.value = required_field(record, key, "value");
	params.pickableType = parse_pickable_type(required_field(record, key, "pickableType").get<std::string>());
	params.baseWeight = required_field(record, key, "baseWeight");
	params.weight = required_field(record, key, "weight");
	params.levelMin = required_field(record, key, "levelMin");
	params.levelMax = required_field(record, key, "levelMax");
	params.levelScaling = required_field(record, key, "levelScaling");
	params.consumableAmount = required_field(record, key, "consumableAmount");
	params.range = required_field(record, key, "range");
	params.damage = required_field(record, key, "damage");
	params.confuseTurns = required_field(record, key, "confuseTurns");
	params.duration = required_field(record, key, "duration");
	params.effect = parse_magical_effect(required_field(record, key, "effect").get<std::string>());
	params.effectBonus = required_field(record, key, "effectBonus");
	params.strBonus = required_field(record, key, "strBonus");
	params.dexBonus = required_field(record, key, "dexBonus");
	params.conBonus = required_field(record, key, "conBonus");
	params.intBonus = required_field(record, key, "intBonus");
	params.wisBonus = required_field(record, key, "wisBonus");
	params.chaBonus = required_field(record, key, "chaBonus");
	params.isSetMode = required_field(record, key, "isSetMode");
	params.exceptionalStrength = required_field(record, key, "exceptionalStrength");
	params.nutritionValue = required_field(record, key, "nutritionValue");
	params.acBonus = required_field(record, key, "acBonus");
	params.ranged = required_field(record, key, "ranged");
	params.handRequirement = parse_hand_requirement(required_field(record, key, "handRequirement").get<std::string>());
	params.weaponSize = parse_weapon_size(required_field(record, key, "weaponSize").get<std::string>());
	params.strengthRating = required_field(record, key, "strengthRating");
	params.consumableEffect = parse_consumable_effect(required_field(record, key, "consumableEffect").get<std::string>());
	params.consumableBuffType = parse_buff_type(required_field(record, key, "consumableBuff").get<std::string>());
	params.targetMode = parse_target_mode(required_field(record, key, "targetMode").get<std::string>());
	params.scrollAnimation = parse_scroll_animation(required_field(record, key, "scrollAnimation").get<std::string>());
	// The name and category views are pointed at the stored entry's strings once it is
	// in the registry; pointed here, they would dangle when this returns.

	return parsed;
}

} // namespace

void ItemRegistry::point_views_at_own_strings(ItemEntry& entry)
{
	entry.params.name = std::string_view{ entry.name };
	entry.params.category = std::string_view{ entry.category };
}

void ItemRegistry::load(std::string_view path)
{
	const auto resolved = Paths::resolve(path);
	std::ifstream file(resolved);
	if (!file.is_open())
	{
		throw std::runtime_error(
			std::format("ItemRegistry::load -- cannot open '{}'", resolved.string()));
	}

	const nlohmann::json root = nlohmann::json::parse(file);
	entries.clear();
	builtinKeys.clear();

	for (const auto& [key, record] : root.items())
	{
		ParsedItem parsed = parse_item(key, record);
		ItemEntry& entry = entries[key];
		entry.name = std::move(parsed.name);
		entry.category = std::move(parsed.category);
		entry.params = parsed.params;
		point_views_at_own_strings(entry);
		builtinKeys.insert(key);
	}
}

void ItemRegistry::save(std::string_view path) const
{
	const auto resolved = Paths::resolve(path);
	std::filesystem::create_directories(resolved.parent_path());

	nlohmann::json root = nlohmann::json::object();
	for (const auto& [key, entry] : entries)
	{
		root[key] = encode_item(entry.params);
	}

	std::ofstream file(resolved);
	if (!file.is_open())
	{
		throw std::runtime_error(
			std::format("ItemRegistry::save -- cannot open '{}' for writing", resolved.string()));
	}
	file << root.dump(4);
	if (file.fail())
	{
		throw std::runtime_error(
			std::format("ItemRegistry::save -- write failed for '{}'", resolved.string()));
	}
}

std::vector<std::string> ItemRegistry::get_all_keys() const
{
	std::vector<std::string> keys;
	keys.reserve(entries.size());
	for (const auto& [key, _] : entries)
	{
		keys.push_back(key);
	}

	return keys;
}

const ItemParams& ItemRegistry::get_params(std::string_view key) const
{
	const auto found = entries.find(key);
	if (found == entries.end())
	{
		throw std::out_of_range(
			std::format("ItemRegistry::get_params -- unknown key '{}'", key));
	}
	return found->second.params;
}

void ItemRegistry::set_params(std::string_view key, const ItemParams& params)
{
	const auto found = entries.find(key);
	if (found == entries.end())
	{
		throw std::out_of_range(
			std::format("ItemRegistry::set_params -- unknown key '{}'", key));
	}
	found->second.params = params;
	point_views_at_own_strings(found->second);
}

void ItemRegistry::set_name_category(std::string_view key, std::string name, std::string category)
{
	const auto found = entries.find(key);
	if (found == entries.end())
	{
		throw std::out_of_range(
			std::format("ItemRegistry::set_name_category -- unknown key '{}'", key));
	}
	found->second.name = std::move(name);
	found->second.category = std::move(category);
	point_views_at_own_strings(found->second);
}

std::string ItemRegistry::add_custom(std::string name, std::string category, ItemParams params)
{
	const std::string base = normalize_key(name.empty() ? "new_item" : name);
	std::string key = base;
	if (entries.contains(key))
	{
		for (int suffix = 2;; ++suffix)
		{
			key = std::format("{}_{}", base, suffix);
			if (!entries.contains(key))
			{
				break;
			}
		}
	}

	ItemEntry& entry = entries[key];
	entry.name = std::move(name);
	entry.category = std::move(category);
	entry.params = params;
	point_views_at_own_strings(entry);

	return key;
}

void ItemRegistry::remove_custom(std::string_view key)
{
	if (is_builtin_key(key))
	{
		throw std::logic_error(
			std::format("ItemRegistry::remove_custom -- '{}' is a built-in item", key));
	}
	if (entries.erase(std::string{ key }) == 0)
	{
		throw std::out_of_range(
			std::format("ItemRegistry::remove_custom -- unknown key '{}'", key));
	}
}

bool ItemRegistry::is_builtin_key(std::string_view key) const
{
	return builtinKeys.contains(std::string{ key });
}

void ItemRegistry::load_enhanced_rules(std::string_view path)
{
	const auto resolved = Paths::resolve(path);
	std::ifstream file(resolved);
	if (!file.is_open())
	{
		throw std::runtime_error(
			std::format("ItemRegistry::load_enhanced_rules -- cannot open '{}'", resolved.string()));
	}

	const nlohmann::json root = nlohmann::json::parse(file);
	enhancedRules.clear();

	for (const auto& entry : root)
	{
		EnhancedItemSpawnRule rule;

		rule.enhancementCategory = parse_enhancement_category(
			entry.at("enhancement_category").get<std::string>());

		rule.baseWeight = entry.at("base_weight").get<int>();
		rule.levelMin = entry.at("level_minimum").get<int>();
		rule.levelMax = entry.at("level_maximum").get<int>();
		rule.levelScaling = entry.at("level_scaling").get<float>();
		rule.category = entry.at("category").get<std::string>();

		for (const auto& key : entry.at("item_pool"))
		{
			rule.itemPool.push_back(key.get<std::string>());
		}

		enhancedRules.push_back(std::move(rule));
	}
}

std::span<const EnhancedItemSpawnRule> ItemRegistry::get_enhanced_rules() const
{
	return enhancedRules;
}
