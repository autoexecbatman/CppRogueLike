#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

#include "Item.h"
#include "Pickable.h"
#include "GameContext.h"
#include "Paths.h"
#include "ItemClassification.h"
#include "MagicalItemEffects.h"
#include "RandomDice.h"
#include "Renderer.h"
#include "BuffType.h"
#include "ContentRegistry.h"
#include "ItemEnhancements.h"
#include "Vector2D.h"
#include "ItemCreator.h"

namespace
{

// ---------------------------------------------------------------------------
// Item entry -- owns name/category strings so ItemParams string_views stay valid
// ---------------------------------------------------------------------------
struct ItemEntry
{
	std::string name{};
	std::string category{};
	ItemParams params{};
};

std::map<std::string, ItemEntry> registry;
std::unordered_set<std::string> builtinKeys;
std::vector<EnhancedItemSpawnRule> enhancedRules;

// ---------------------------------------------------------------------------
// Key helpers
// ---------------------------------------------------------------------------
std::string normalize_key(std::string_view raw)
{
	std::string key;
	key.reserve(raw.size());
	for (char character : raw)
	{
		if (std::isspace(static_cast<unsigned char>(character)))
			key += '_';
		else
			key += static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
	}
	return key;
}

std::string unique_item_key(const std::string& base)
{
	if (!registry.contains(base))
		return base;
	for (int suffix = 2;; ++suffix)
	{
		std::string candidate = std::format("{}_{}", base, suffix);
		if (!registry.contains(candidate))
			return candidate;
	}
}

void patch_views(ItemEntry& entry)
{
	entry.params.name = std::string_view{ entry.name };
	entry.params.category = std::string_view{ entry.category };
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

	throw std::runtime_error(std::format("ItemCreator: unknown consumable_effect '{}'", name));
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
nlohmann::json encode_item_entry(const ItemEntry& entry)
{
	const ItemParams& params = entry.params;
	nlohmann::json record;
	record["name"] = entry.name;
	record["category"] = entry.category;
	record["color"] = params.color;
	record["itemClass"] = encode_item_class(params.itemClass);
	record["value"] = params.value;
	record["pickableType"] = encode_pickable_type(params.pickableType);
	record["baseWeight"] = params.baseWeight;
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
	record["nutritionValue"] = params.nutritionValue;
	record["goldAmount"] = params.goldAmount;
	record["acBonus"] = params.acBonus;
	record["ranged"] = params.ranged;
	record["handRequirement"] = encode_hand_requirement(params.handRequirement);
	record["weaponSize"] = encode_weapon_size(params.weaponSize);
	record["consumableEffect"] = encode_consumable_effect(params.consumableEffect);
	record["consumableBuff"] = encode_buff_type(params.consumableBuffType);
	record["targetMode"] = encode_target_mode(params.targetMode);
	record["scrollAnimation"] = encode_scroll_animation(params.scrollAnimation);

	return record;
}

// Reads a field every item record carries. encode_item_entry writes all thirty-four
// unconditionally, so a record missing one is a corrupted or hand-edited file rather
// than a shorter record - this names the item and the field instead of handing back a
// default nobody authored.
//
// Example:
//   required_field(record, "health_potion", "baseWeight");   // -> 12
//   required_field(record, "health_potion", "weight");
//   // throws: ItemCreator::load -- item 'health_potion' is missing required field 'weight'
const nlohmann::json& required_field(
	const nlohmann::json& record,
	const std::string& itemKey,
	const std::string& field)
{
	const auto found = record.find(field);
	if (found == record.end())
	{
		throw std::runtime_error(std::format(
			"ItemCreator::load -- item '{}' is missing required field '{}'",
			itemKey,
			field));
	}
	return *found;
}

ItemEntry parse_item_entry(const std::string& key, const nlohmann::json& record)
{
	ItemEntry entry;
	entry.name = required_field(record, key, "name").get<std::string>();
	entry.category = required_field(record, key, "category").get<std::string>();

	ItemParams& params = entry.params;
	params.color = required_field(record, key, "color");
	params.itemClass = parse_item_class(required_field(record, key, "itemClass").get<std::string>());
	params.value = required_field(record, key, "value");
	params.pickableType = parse_pickable_type(required_field(record, key, "pickableType").get<std::string>());
	params.baseWeight = required_field(record, key, "baseWeight");
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
	params.nutritionValue = required_field(record, key, "nutritionValue");
	params.goldAmount = required_field(record, key, "goldAmount");
	params.acBonus = required_field(record, key, "acBonus");
	params.ranged = required_field(record, key, "ranged");
	params.handRequirement = parse_hand_requirement(required_field(record, key, "handRequirement").get<std::string>());
	params.weaponSize = parse_weapon_size(required_field(record, key, "weaponSize").get<std::string>());
	params.consumableEffect = parse_consumable_effect(required_field(record, key, "consumableEffect").get<std::string>());
	params.consumableBuffType = parse_buff_type(required_field(record, key, "consumableBuff").get<std::string>());
	params.targetMode = parse_target_mode(required_field(record, key, "targetMode").get<std::string>());
	params.scrollAnimation = parse_scroll_animation(required_field(record, key, "scrollAnimation").get<std::string>());
	// NOTE: do NOT call patch_views here -- views would dangle after return-by-value.
	// Caller must call patch_views after placing the entry in its stable storage.

	return entry;
}

// ---------------------------------------------------------------------------
// Behavior construction
// ---------------------------------------------------------------------------
template <typename T>
T create_stat_behavior(const ItemParams& params)
{
	T item;
	item.strBonus = params.strBonus;
	item.dexBonus = params.dexBonus;
	item.conBonus = params.conBonus;
	item.intBonus = params.intBonus;
	item.wisBonus = params.wisBonus;
	item.chaBonus = params.chaBonus;
	item.effect = params.effect;
	item.bonus = params.effectBonus;
	item.isSetMode = params.isSetMode;
	return item;
}

ItemBehavior create_behavior(const ItemParams& params)
{
	switch (params.pickableType)
	{

	case PickableType::CONSUMABLE:
	{
		return Consumable{
			params.consumableEffect,
			params.consumableAmount,
			params.duration,
			params.consumableBuffType,
			params.isSetMode
		};
	}

	case PickableType::TARGETED_SCROLL:
	{
		return TargetedScroll{
			params.targetMode,
			params.scrollAnimation,
			params.range,
			params.damage,
			params.confuseTurns,
			params.consumableBuffType,
			params.duration
		};
	}

	case PickableType::TELEPORTER:
	{
		return Teleporter{};
	}

	case PickableType::IDENTIFY_SCROLL:
	{
		return IdentifyScroll{};
	}

	case PickableType::WEAPON:
	{
		return Weapon{ params.ranged, params.handRequirement, params.weaponSize };
	}

	case PickableType::SHIELD:
	{
		return Shield{};
	}

	case PickableType::ARMOR:
	{
		return Armor{ params.acBonus };
	}

	case PickableType::QUEST_ITEM:
	{
		return Amulet{};
	}

	case PickableType::MAGICAL_HELM:
	{
		return MagicalHelm{ params.effect, params.effectBonus };
	}

	case PickableType::MAGICAL_RING:
	{
		return MagicalRing{ params.effect, params.effectBonus };
	}

	case PickableType::JEWELRY_AMULET:
	{
		return create_stat_behavior<JewelryAmulet>(params);
	}

	case PickableType::GAUNTLETS:
	{
		return create_stat_behavior<Gauntlets>(params);
	}

	case PickableType::GIRDLE:
	{
		return create_stat_behavior<Girdle>(params);
	}

	case PickableType::FOOD:
	{
		return Food{ params.nutritionValue };
	}

	case PickableType::GOLD_COIN:
	{
		return Gold{ 0 };
	}

	case PickableType::DUNGEON_KEY:
	{
		return DungeonKey{};
	}

	default:
	{
		throw std::runtime_error(
			std::format("ItemCreator: unhandled pickable_type {}",
				static_cast<int>(params.pickableType)));
	}

	}
}

std::unique_ptr<Item> make_item(std::string_view key, const ItemEntry& entry, Vector2D pos, ContentRegistry& registry)
{
	const ItemParams& params = entry.params;
	TileRef tile = registry.get_tile(key);
	auto item = std::make_unique<Item>(
		pos,
		ActorData{ tile, std::string{ params.name }, params.color });
	item->behavior = create_behavior(params);
	item->itemKey = std::string{ key };
	item->itemClass = params.itemClass;
	item->set_value(params.value);
	item->enhancement.weight = params.baseWeight; // Assign base weight to enhancement
	return item;
}

} // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void ItemCreator::load(std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::ifstream file(resolved);
	if (!file.is_open())
	{
		return;
	}

	nlohmann::json root = nlohmann::json::parse(file);
	registry.clear();
	builtinKeys.clear();

	for (const auto& [key, record] : root.items())
	{
		registry[key] = parse_item_entry(key, record);
		patch_views(registry[key]); // patch after stable insertion into map
		builtinKeys.insert(key);
	}
}

void ItemCreator::save(std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::filesystem::create_directories(resolved.parent_path());

	nlohmann::json root = nlohmann::json::object();
	for (const auto& [key, entry] : registry)
	{
		root[key] = encode_item_entry(entry);
	}

	std::ofstream file(resolved);
	if (!file.is_open())
	{
		throw std::runtime_error(
			std::format("ItemCreator::save -- cannot open '{}' for writing", resolved.string()));
	}
	file << root.dump(4);
	if (file.fail())
	{
		throw std::runtime_error(
			std::format("ItemCreator::save -- write failed for '{}'", resolved.string()));
	}
}

std::vector<std::string> ItemCreator::get_all_keys()
{
	std::vector<std::string> keys;
	keys.reserve(registry.size());
	for (const auto& [key, _] : registry)
	{
		keys.push_back(key);
	}

	return keys;
}

const ItemParams& ItemCreator::get_params(std::string_view key)
{
	if (!registry.contains(std::string{ key }))
	{
		throw std::out_of_range(
			std::format("ItemCreator::get_params -- unknown key '{}'", key));
	}
	return registry.at(std::string{ key }).params;
}

void ItemCreator::set_params(std::string_view key, const ItemParams& params)
{
	if (!registry.contains(std::string{ key }))
	{
		throw std::out_of_range(
			std::format("ItemCreator::set_params -- unknown key '{}'", key));
	}
	auto& entry = registry.at(std::string{ key });
	entry.params = params;
	patch_views(entry);
}

std::unique_ptr<Item> ItemCreator::create(std::string_view key, Vector2D pos, ContentRegistry& tiles)
{
	if (!registry.contains(std::string{ key }))
	{
		throw std::out_of_range(
			std::format("ItemCreator::create -- unknown key '{}'", key));
	}
	return make_item(key, registry.at(std::string{ key }), pos, tiles);
}

std::unique_ptr<Item> ItemCreator::create_with_gold_amount(Vector2D pos, int goldAmount, ContentRegistry& tiles)
{
	if (!registry.contains("gold_coin"))
	{
		throw std::runtime_error(
			"ItemCreator::create_with_gold_amount -- 'gold_coin' not in registry");
	}
	const ItemParams& params = registry.at("gold_coin").params;
	TileRef tile = tiles.get_tile("gold_coin");
	auto item = std::make_unique<Item>(
		pos,
		ActorData{ tile, std::string{ params.name }, params.color });
	item->behavior = Gold{ goldAmount };
	item->itemKey = "gold_coin";
	item->itemClass = params.itemClass;
	item->set_value(goldAmount);
	return item;
}

std::unique_ptr<Item> ItemCreator::create_with_enhancement(
	std::string_view key,
	Vector2D pos,
	PrefixType prefix,
	SuffixType suffix,
	ContentRegistry& tiles)
{
	auto item = create(key, pos, tiles);
	item->apply_enhancement(ItemEnhancement(prefix, suffix));

	return item;
}

std::unique_ptr<Item> ItemCreator::create_gold_pile(Vector2D pos, GameContext& ctx)
{
	const int goldAmount = ctx.dice->roll(5, 20);

	return create_with_gold_amount(pos, goldAmount, *ctx.contentRegistry);
}

std::unique_ptr<Item> ItemCreator::create_random_of_category(
	std::string_view category,
	Vector2D pos,
	GameContext& ctx,
	int dungeonLevel)
{
	struct Candidate
	{
		std::string key;
		int weight;
	};

	std::vector<Candidate> candidates;
	int totalWeight = 0;

	for (const auto& [key, entry] : registry)
	{
		const ItemParams& params = entry.params;
		if (params.category != category || params.baseWeight <= 0)
		{
			continue;
		}
		if (dungeonLevel < params.levelMin)
		{
			continue;
		}
		if (params.levelMax > 0 && dungeonLevel > params.levelMax)
		{
			continue;
		}

		const float levelFactor = 1.0f + (params.levelScaling * static_cast<float>(dungeonLevel - 1));
		const int weight = std::max(1, static_cast<int>(params.baseWeight * levelFactor));
		candidates.push_back({ key, weight });
		totalWeight += weight;
	}

	if (candidates.empty())
	{
		return nullptr;
	}

	int roll = ctx.dice->roll(1, totalWeight);
	for (const auto& [key, weight] : candidates)
	{
		roll -= weight;
		if (roll <= 0)
		{
			return create(key, pos, *ctx.contentRegistry);
		}
	}

	return create(candidates.back().key, pos, *ctx.contentRegistry);
}

std::string ItemCreator::add_custom(std::string name, std::string category, ItemParams params)
{
	const std::string key = unique_item_key(normalize_key(name.empty() ? "new_item" : name));
	ItemEntry& entry = registry[key];
	entry.name = std::move(name);
	entry.category = std::move(category);
	entry.params = params;
	patch_views(entry);

	return key;
}

void ItemCreator::remove_custom(std::string_view key)
{
	if (builtinKeys.contains(std::string{ key }))
	{
		throw std::logic_error(
			std::format("ItemCreator::remove_custom -- '{}' is a built-in item", key));
	}
	if (!registry.erase(std::string{ key }))
	{
		throw std::out_of_range(
			std::format("ItemCreator::remove_custom -- unknown key '{}'", key));
	}
}

void ItemCreator::set_name_category(std::string_view key, std::string name, std::string category)
{
	if (!registry.contains(std::string{ key }))
	{
		throw std::out_of_range(
			std::format("ItemCreator::set_name_category -- unknown key '{}'", key));
	}
	auto& entry = registry.at(std::string{ key });
	entry.name = std::move(name);
	entry.category = std::move(category);
	patch_views(entry);
}

bool ItemCreator::is_builtin_key(std::string_view key)
{
	return builtinKeys.contains(std::string{ key });
}

void ItemCreator::load_enhanced_rules(std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::ifstream file(resolved);
	if (!file.is_open())
	{
		return;
	}

	nlohmann::json root = nlohmann::json::parse(file);
	enhancedRules.clear();

	for (const auto& entry : root)
	{
		EnhancedItemSpawnRule rule;

		std::string category = entry.at("enhancement_category").get<std::string>();
		rule.enhancementCategory = (category == "weapon")
			? EnhancedItemCategory::WEAPON
			: EnhancedItemCategory::ARMOR;

		rule.baseWeight = entry.at("base_weight").get<int>();
		rule.levelMin = entry.at("level_minimum").get<int>();
		rule.levelMax = entry.value("level_maximum", 0);
		rule.levelScaling = entry.at("level_scaling").get<float>();
		rule.category = entry.at("category").get<std::string>();

		for (const auto& key : entry.at("item_pool"))
		{
			rule.itemPool.push_back(key.get<std::string>());
		}

		enhancedRules.push_back(std::move(rule));
	}
}

std::span<const EnhancedItemSpawnRule> ItemCreator::get_enhanced_rules()
{
	return enhancedRules;
}
