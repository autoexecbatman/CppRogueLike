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

#include "../Actor/Item.h"
#include "../Actor/Pickable.h"
#include "../Core/GameContext.h"
#include "../Core/Paths.h"
#include "../Items/ItemClassification.h"
#include "../Items/MagicalItemEffects.h"
#include "../Random/RandomDice.h"
#include "../Renderer/Renderer.h"
#include "../Systems/BuffType.h"
#include "../Systems/ContentRegistry.h"
#include "../Systems/ItemEnhancements/ItemEnhancements.h"
#include "../Utils/Vector2D.h"
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
	for (char c : raw)
	{
		if (std::isspace(static_cast<unsigned char>(c)))
			key += '_';
		else
			key += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return key;
}

std::string unique_item_key(const std::string& base)
{
	if (!registry.contains(base))
		return base;
	for (int n = 2;; ++n)
	{
		std::string candidate = std::format("{}_{}", base, n);
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
ConsumableEffect parse_consumable_effect(std::string_view s)
{
	if (s == "none")
	{
		return ConsumableEffect::NONE;
	}
	if (s == "heal")
	{
		return ConsumableEffect::HEAL;
	}
	if (s == "add_buff")
	{
		return ConsumableEffect::ADD_BUFF;
	}
	if (s == "fail")
	{
		return ConsumableEffect::FAIL;
	}

	throw std::runtime_error(std::format("ItemCreator: unknown consumable_effect '{}'", s));
}

// ---------------------------------------------------------------------------
// Enum encode helpers
// ---------------------------------------------------------------------------
std::string_view encode_consumable_effect(ConsumableEffect e)
{
	switch (e)
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
nlohmann::json encode_item_entry(const ItemEntry& e)
{
	const ItemParams& p = e.params;
	nlohmann::json j;
	j["name"] = e.name;
	j["category"] = e.category;
	j["color"] = p.color;
	j["itemClass"] = encode_item_class(p.itemClass);
	j["value"] = p.value;
	j["pickableType"] = encode_pickable_type(p.pickableType);
	j["baseWeight"] = p.baseWeight;
	j["levelMin"] = p.levelMin;
	j["levelMax"] = p.levelMax;
	j["levelScaling"] = p.levelScaling;
	j["consumableAmount"] = p.consumableAmount;
	j["range"] = p.range;
	j["damage"] = p.damage;
	j["confuseTurns"] = p.confuseTurns;
	j["duration"] = p.duration;
	j["effect"] = encode_magical_effect(p.effect);
	j["effectBonus"] = p.effectBonus;
	j["strBonus"] = p.strBonus;
	j["dexBonus"] = p.dexBonus;
	j["conBonus"] = p.conBonus;
	j["intBonus"] = p.intBonus;
	j["wisBonus"] = p.wisBonus;
	j["chaBonus"] = p.chaBonus;
	j["isSetMode"] = p.isSetMode;
	j["nutritionValue"] = p.nutritionValue;
	j["goldAmount"] = p.goldAmount;
	j["acBonus"] = p.acBonus;
	j["ranged"] = p.ranged;
	j["handRequirement"] = encode_hand_requirement(p.handRequirement);
	j["weaponSize"] = encode_weapon_size(p.weaponSize);
	j["consumableEffect"] = encode_consumable_effect(p.consumableEffect);
	j["consumableBuff"] = encode_buff_type(p.consumableBuffType);
	j["targetMode"] = encode_target_mode(p.targetMode);
	j["scrollAnimation"] = encode_scroll_animation(p.scrollAnimation);

	return j;
}

ItemEntry parse_item_entry(const std::string& key, const nlohmann::json& j)
{
	ItemEntry e;
	e.name = j.value("name", key);
	e.category = j.value("category", std::string{});

	ItemParams& p = e.params;
	p.color = j.value("color", 0);
	p.itemClass = parse_item_class(j.value("itemClass", std::string{ "unknown" }));
	p.value = j.value("value", 0);
	p.pickableType = parse_pickable_type(j.value("pickableType", std::string{ "weapon" }));
	p.baseWeight = j.value("baseWeight", 0);
	p.levelMin = j.value("levelMin", 1);
	p.levelMax = j.value("levelMax", 0);
	p.levelScaling = j.value("levelScaling", 0.0f);
	p.consumableAmount = j.value("consumableAmount", 0);
	p.range = j.value("range", 0);
	p.damage = j.value("damage", 0);
	p.confuseTurns = j.value("confuseTurns", 0);
	p.duration = j.value("duration", 0);
	p.effect = parse_magical_effect(j.value("effect", std::string{ "none" }));
	p.effectBonus = j.value("effectBonus", 0);
	p.strBonus = j.value("strBonus", 0);
	p.dexBonus = j.value("dexBonus", 0);
	p.conBonus = j.value("conBonus", 0);
	p.intBonus = j.value("intBonus", 0);
	p.wisBonus = j.value("wisBonus", 0);
	p.chaBonus = j.value("chaBonus", 0);
	p.isSetMode = j.value("isSetMode", false);
	p.nutritionValue = j.value("nutritionValue", 0);
	p.goldAmount = j.value("goldAmount", 0);
	p.acBonus = j.value("acBonus", 0);
	p.ranged = j.value("ranged", false);
	p.handRequirement = parse_hand_requirement(j.value("handRequirement", std::string{ "one_handed" }));
	p.weaponSize = parse_weapon_size(j.value("weaponSize", std::string{ "medium" }));
	p.consumableEffect = parse_consumable_effect(j.value("consumableEffect", std::string{ "none" }));
	p.consumableBuffType = parse_buff_type(j.value("consumableBuff", std::string{ "none" }));
	p.targetMode = parse_target_mode(j.value("targetMode", std::string{ "auto_nearest" }));
	p.scrollAnimation = parse_scroll_animation(j.value("scrollAnimation", std::string{ "none" }));
	// NOTE: do NOT call patch_views here -- views would dangle after return-by-value.
	// Caller must call patch_views after placing the entry in its stable storage.

	return e;
}

// ---------------------------------------------------------------------------
// Behavior construction
// ---------------------------------------------------------------------------
template <typename T>
T create_stat_behavior(const ItemParams& p)
{
	T item;
	item.strBonus = p.strBonus;
	item.dexBonus = p.dexBonus;
	item.conBonus = p.conBonus;
	item.intBonus = p.intBonus;
	item.wisBonus = p.wisBonus;
	item.chaBonus = p.chaBonus;
	item.effect = p.effect;
	item.bonus = p.effectBonus;
	item.isSetMode = p.isSetMode;
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
	const ItemParams& p = entry.params;
	TileRef tile = registry.get_tile(key);
	auto item = std::make_unique<Item>(
		pos,
		ActorData{ tile, std::string{ p.name }, p.color });
	item->behavior = create_behavior(p);
	item->itemKey = std::string{ key };
	item->itemClass = p.itemClass;
	item->set_value(p.value);
	item->enhancement.weight = p.baseWeight; // Assign base weight to enhancement
	return item;
}

} // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void ItemCreator::load(std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::ifstream f(resolved);
	if (!f.is_open())
	{
		return;
	}

	nlohmann::json root = nlohmann::json::parse(f);
	registry.clear();
	builtinKeys.clear();

	for (const auto& [key, val] : root.items())
	{
		registry[key] = parse_item_entry(key, val);
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

	std::ofstream f(resolved);
	if (!f.is_open())
	{
		throw std::runtime_error(
			std::format("ItemCreator::save -- cannot open '{}' for writing", resolved.string()));
	}
	f << root.dump(4);
	if (f.fail())
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
	const ItemParams& p = registry.at("gold_coin").params;
	TileRef tile = tiles.get_tile("gold_coin");
	auto item = std::make_unique<Item>(
		pos,
		ActorData{ tile, std::string{ p.name }, p.color });
	item->behavior = Gold{ goldAmount };
	item->itemKey = "gold_coin";
	item->itemClass = p.itemClass;
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
		const ItemParams& p = entry.params;
		if (p.category != category || p.baseWeight <= 0)
		{
			continue;
		}
		if (dungeonLevel < p.levelMin)
		{
			continue;
		}
		if (p.levelMax > 0 && dungeonLevel > p.levelMax)
		{
			continue;
		}

		const float levelFactor = 1.0f + (p.levelScaling * static_cast<float>(dungeonLevel - 1));
		const int weight = std::max(1, static_cast<int>(p.baseWeight * levelFactor));
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
	std::ifstream f(resolved);
	if (!f.is_open())
	{
		return;
	}

	nlohmann::json root = nlohmann::json::parse(f);
	enhancedRules.clear();

	for (const auto& entry : root)
	{
		EnhancedItemSpawnRule rule;

		std::string cat = entry.at("enhancement_category").get<std::string>();
		rule.enhancementCategory = (cat == "weapon")
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
