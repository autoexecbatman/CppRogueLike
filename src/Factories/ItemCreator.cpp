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
#include "../Random/RandomDice.h"
#include "../Renderer/Renderer.h"
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
	std::string name;
	std::string category;
	ItemParams params;
};

std::map<std::string, ItemEntry> s_registry;
std::unordered_set<std::string> s_builtin_keys;
std::vector<EnhancedItemSpawnRule> s_enhanced_rules;

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
	if (!s_registry.contains(base))
		return base;
	for (int n = 2; ; ++n)
	{
		std::string candidate = std::format("{}_{}", base, n);
		if (!s_registry.contains(candidate))
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
PickableType parse_pickable_type(std::string_view s)
{
	if (s == "targeted_scroll") return PickableType::TARGETED_SCROLL;
	if (s == "teleporter")      return PickableType::TELEPORTER;
	if (s == "weapon")          return PickableType::WEAPON;
	if (s == "shield")          return PickableType::SHIELD;
	if (s == "consumable")      return PickableType::CONSUMABLE;
	if (s == "gold_coin")       return PickableType::GOLD_COIN;
	if (s == "food")            return PickableType::FOOD;
	if (s == "corpse_food")     return PickableType::CORPSE_FOOD;
	if (s == "armor")           return PickableType::ARMOR;
	if (s == "magical_helm")    return PickableType::MAGICAL_HELM;
	if (s == "magical_ring")    return PickableType::MAGICAL_RING;
	if (s == "jewelry_amulet")  return PickableType::JEWELRY_AMULET;
	if (s == "gauntlets")       return PickableType::GAUNTLETS;
	if (s == "girdle")          return PickableType::GIRDLE;
	if (s == "quest_item")      return PickableType::QUEST_ITEM;
	throw std::runtime_error(std::format("ItemCreator: unknown pickable_type '{}'", s));
}

ItemClass parse_item_class(std::string_view s)
{
	if (s == "unknown")     return ItemClass::UNKNOWN;
	if (s == "dagger")      return ItemClass::DAGGER;
	if (s == "sword")       return ItemClass::SWORD;
	if (s == "great_sword") return ItemClass::GREAT_SWORD;
	if (s == "axe")         return ItemClass::AXE;
	if (s == "hammer")      return ItemClass::HAMMER;
	if (s == "mace")        return ItemClass::MACE;
	if (s == "staff")       return ItemClass::STAFF;
	if (s == "bow")         return ItemClass::BOW;
	if (s == "crossbow")    return ItemClass::CROSSBOW;
	if (s == "armor")       return ItemClass::ARMOR;
	if (s == "shield")      return ItemClass::SHIELD;
	if (s == "helmet")      return ItemClass::HELMET;
	if (s == "ring")        return ItemClass::RING;
	if (s == "amulet")      return ItemClass::AMULET;
	if (s == "gauntlets")   return ItemClass::GAUNTLETS;
	if (s == "girdle")      return ItemClass::GIRDLE;
	if (s == "potion")      return ItemClass::POTION;
	if (s == "scroll")      return ItemClass::SCROLL;
	if (s == "food")        return ItemClass::FOOD;
	if (s == "gold_coin")   return ItemClass::GOLD_COIN;
	if (s == "gem")         return ItemClass::GEM;
	if (s == "tool")        return ItemClass::TOOL;
	if (s == "quest_item")  return ItemClass::QUEST_ITEM;
	throw std::runtime_error(std::format("ItemCreator: unknown item_class '{}'", s));
}

HandRequirement parse_hand_requirement(std::string_view s)
{
	if (s == "one_handed")    return HandRequirement::ONE_HANDED;
	if (s == "two_handed")    return HandRequirement::TWO_HANDED;
	if (s == "off_hand_only") return HandRequirement::OFF_HAND_ONLY;
	throw std::runtime_error(std::format("ItemCreator: unknown hand_requirement '{}'", s));
}

WeaponSize parse_weapon_size(std::string_view s)
{
	if (s == "tiny")   return WeaponSize::TINY;
	if (s == "small")  return WeaponSize::SMALL;
	if (s == "medium") return WeaponSize::MEDIUM;
	if (s == "large")  return WeaponSize::LARGE;
	if (s == "giant")  return WeaponSize::GIANT;
	throw std::runtime_error(std::format("ItemCreator: unknown weapon_size '{}'", s));
}

ConsumableEffect parse_consumable_effect(std::string_view s)
{
	if (s == "none")     return ConsumableEffect::NONE;
	if (s == "heal")     return ConsumableEffect::HEAL;
	if (s == "add_buff") return ConsumableEffect::ADD_BUFF;
	if (s == "fail")     return ConsumableEffect::FAIL;
	throw std::runtime_error(std::format("ItemCreator: unknown consumable_effect '{}'", s));
}

MagicalEffect parse_magical_effect(std::string_view s)
{
	if (s == "none")              return MagicalEffect::NONE;
	if (s == "brilliance")        return MagicalEffect::BRILLIANCE;
	if (s == "teleportation")     return MagicalEffect::TELEPORTATION;
	if (s == "telepathy")         return MagicalEffect::TELEPATHY;
	if (s == "underwater_action") return MagicalEffect::UNDERWATER_ACTION;
	if (s == "free_action")       return MagicalEffect::FREE_ACTION;
	if (s == "regeneration")      return MagicalEffect::REGENERATION;
	if (s == "invisibility")      return MagicalEffect::INVISIBILITY;
	if (s == "fire_resistance")   return MagicalEffect::FIRE_RESISTANCE;
	if (s == "cold_resistance")   return MagicalEffect::COLD_RESISTANCE;
	if (s == "spell_storing")     return MagicalEffect::SPELL_STORING;
	if (s == "protection")        return MagicalEffect::PROTECTION;
	throw std::runtime_error(std::format("ItemCreator: unknown magical_effect '{}'", s));
}

TargetMode parse_target_mode(std::string_view s)
{
	if (s == "auto_nearest")     return TargetMode::AUTO_NEAREST;
	if (s == "pick_tile_single") return TargetMode::PICK_TILE_SINGLE;
	if (s == "pick_tile_aoe")    return TargetMode::PICK_TILE_AOE;
	if (s == "fov_buff")         return TargetMode::FOV_BUFF;
	throw std::runtime_error(std::format("ItemCreator: unknown target_mode '{}'", s));
}

ScrollAnimation parse_scroll_animation(std::string_view s)
{
	if (s == "none")      return ScrollAnimation::NONE;
	if (s == "lightning") return ScrollAnimation::LIGHTNING;
	if (s == "explosion") return ScrollAnimation::EXPLOSION;
	throw std::runtime_error(std::format("ItemCreator: unknown scroll_animation '{}'", s));
}

BuffType parse_buff_type(std::string_view s)
{
	if (s == "none")                 return BuffType::NONE;
	if (s == "invisibility")         return BuffType::INVISIBILITY;
	if (s == "bless")                return BuffType::BLESS;
	if (s == "shield")               return BuffType::SHIELD;
	if (s == "strength")             return BuffType::STRENGTH;
	if (s == "dexterity")            return BuffType::DEXTERITY;
	if (s == "constitution")         return BuffType::CONSTITUTION;
	if (s == "intelligence")         return BuffType::INTELLIGENCE;
	if (s == "wisdom")               return BuffType::WISDOM;
	if (s == "charisma")             return BuffType::CHARISMA;
	if (s == "speed")                return BuffType::SPEED;
	if (s == "fire_resistance")      return BuffType::FIRE_RESISTANCE;
	if (s == "cold_resistance")      return BuffType::COLD_RESISTANCE;
	if (s == "lightning_resistance") return BuffType::LIGHTNING_RESISTANCE;
	if (s == "poison_resistance")    return BuffType::POISON_RESISTANCE;
	if (s == "sleep")                return BuffType::SLEEP;
	if (s == "hold_person")          return BuffType::HOLD_PERSON;
	throw std::runtime_error(std::format("ItemCreator: unknown buff_type '{}'", s));
}

// ---------------------------------------------------------------------------
// Enum encode helpers
// ---------------------------------------------------------------------------
std::string_view encode_pickable_type(PickableType t)
{
	switch (t)
	{
	case PickableType::TARGETED_SCROLL: return "targeted_scroll";
	case PickableType::TELEPORTER:      return "teleporter";
	case PickableType::WEAPON:          return "weapon";
	case PickableType::SHIELD:          return "shield";
	case PickableType::CONSUMABLE:      return "consumable";
	case PickableType::GOLD_COIN:       return "gold_coin";
	case PickableType::FOOD:            return "food";
	case PickableType::CORPSE_FOOD:     return "corpse_food";
	case PickableType::ARMOR:           return "armor";
	case PickableType::MAGICAL_HELM:    return "magical_helm";
	case PickableType::MAGICAL_RING:    return "magical_ring";
	case PickableType::JEWELRY_AMULET:  return "jewelry_amulet";
	case PickableType::GAUNTLETS:       return "gauntlets";
	case PickableType::GIRDLE:          return "girdle";
	case PickableType::QUEST_ITEM:      return "quest_item";
	}
	return "weapon";
}

std::string_view encode_item_class(ItemClass c)
{
	switch (c)
	{
	case ItemClass::UNKNOWN:    return "unknown";
	case ItemClass::DAGGER:     return "dagger";
	case ItemClass::SWORD:      return "sword";
	case ItemClass::GREAT_SWORD: return "great_sword";
	case ItemClass::AXE:        return "axe";
	case ItemClass::HAMMER:     return "hammer";
	case ItemClass::MACE:       return "mace";
	case ItemClass::STAFF:      return "staff";
	case ItemClass::BOW:        return "bow";
	case ItemClass::CROSSBOW:   return "crossbow";
	case ItemClass::ARMOR:      return "armor";
	case ItemClass::SHIELD:     return "shield";
	case ItemClass::HELMET:     return "helmet";
	case ItemClass::RING:       return "ring";
	case ItemClass::AMULET:     return "amulet";
	case ItemClass::GAUNTLETS:  return "gauntlets";
	case ItemClass::GIRDLE:     return "girdle";
	case ItemClass::POTION:     return "potion";
	case ItemClass::SCROLL:     return "scroll";
	case ItemClass::FOOD:       return "food";
	case ItemClass::GOLD_COIN:  return "gold_coin";
	case ItemClass::GEM:        return "gem";
	case ItemClass::TOOL:       return "tool";
	case ItemClass::QUEST_ITEM: return "quest_item";
	}
	return "unknown";
}

std::string_view encode_hand_requirement(HandRequirement h)
{
	switch (h)
	{
	case HandRequirement::ONE_HANDED:    return "one_handed";
	case HandRequirement::TWO_HANDED:    return "two_handed";
	case HandRequirement::OFF_HAND_ONLY: return "off_hand_only";
	}
	return "one_handed";
}

std::string_view encode_weapon_size(WeaponSize s)
{
	switch (s)
	{
	case WeaponSize::TINY:   return "tiny";
	case WeaponSize::SMALL:  return "small";
	case WeaponSize::MEDIUM: return "medium";
	case WeaponSize::LARGE:  return "large";
	case WeaponSize::GIANT:  return "giant";
	}
	return "medium";
}

std::string_view encode_consumable_effect(ConsumableEffect e)
{
	switch (e)
	{
	case ConsumableEffect::NONE:     return "none";
	case ConsumableEffect::HEAL:     return "heal";
	case ConsumableEffect::ADD_BUFF: return "add_buff";
	case ConsumableEffect::FAIL:     return "fail";
	}
	return "none";
}

std::string_view encode_magical_effect(MagicalEffect e)
{
	switch (e)
	{
	case MagicalEffect::NONE:              return "none";
	case MagicalEffect::BRILLIANCE:        return "brilliance";
	case MagicalEffect::TELEPORTATION:     return "teleportation";
	case MagicalEffect::TELEPATHY:         return "telepathy";
	case MagicalEffect::UNDERWATER_ACTION: return "underwater_action";
	case MagicalEffect::FREE_ACTION:       return "free_action";
	case MagicalEffect::REGENERATION:      return "regeneration";
	case MagicalEffect::INVISIBILITY:      return "invisibility";
	case MagicalEffect::FIRE_RESISTANCE:   return "fire_resistance";
	case MagicalEffect::COLD_RESISTANCE:   return "cold_resistance";
	case MagicalEffect::SPELL_STORING:     return "spell_storing";
	case MagicalEffect::PROTECTION:        return "protection";
	}
	return "none";
}

std::string_view encode_target_mode(TargetMode m)
{
	switch (m)
	{
	case TargetMode::AUTO_NEAREST:     return "auto_nearest";
	case TargetMode::PICK_TILE_SINGLE: return "pick_tile_single";
	case TargetMode::PICK_TILE_AOE:    return "pick_tile_aoe";
	case TargetMode::FOV_BUFF:         return "fov_buff";
	}
	return "auto_nearest";
}

std::string_view encode_scroll_animation(ScrollAnimation a)
{
	switch (a)
	{
	case ScrollAnimation::NONE:      return "none";
	case ScrollAnimation::LIGHTNING: return "lightning";
	case ScrollAnimation::EXPLOSION: return "explosion";
	}
	return "none";
}

std::string_view encode_buff_type(BuffType b)
{
	switch (b)
	{
	case BuffType::NONE:                 return "none";
	case BuffType::INVISIBILITY:         return "invisibility";
	case BuffType::BLESS:                return "bless";
	case BuffType::SHIELD:               return "shield";
	case BuffType::STRENGTH:             return "strength";
	case BuffType::DEXTERITY:            return "dexterity";
	case BuffType::CONSTITUTION:         return "constitution";
	case BuffType::INTELLIGENCE:         return "intelligence";
	case BuffType::WISDOM:               return "wisdom";
	case BuffType::CHARISMA:             return "charisma";
	case BuffType::SPEED:                return "speed";
	case BuffType::FIRE_RESISTANCE:      return "fire_resistance";
	case BuffType::COLD_RESISTANCE:      return "cold_resistance";
	case BuffType::LIGHTNING_RESISTANCE: return "lightning_resistance";
	case BuffType::POISON_RESISTANCE:    return "poison_resistance";
	case BuffType::SLEEP:                return "sleep";
	case BuffType::HOLD_PERSON:          return "hold_person";
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
	j["name"]              = e.name;
	j["category"]          = e.category;
	j["color"]             = p.color;
	j["item_class"]        = encode_item_class(p.itemClass);
	j["value"]             = p.value;
	j["pickable_type"]     = encode_pickable_type(p.pickable_type);
	j["base_weight"]       = p.base_weight;
	j["level_minimum"]     = p.level_minimum;
	j["level_maximum"]     = p.level_maximum;
	j["level_scaling"]     = p.level_scaling;
	j["consumable_amount"] = p.consumable_amount;
	j["range"]             = p.range;
	j["damage"]            = p.damage;
	j["confuse_turns"]     = p.confuse_turns;
	j["duration"]          = p.duration;
	j["effect"]            = encode_magical_effect(p.effect);
	j["effect_bonus"]      = p.effect_bonus;
	j["str_bonus"]         = p.str_bonus;
	j["dex_bonus"]         = p.dex_bonus;
	j["con_bonus"]         = p.con_bonus;
	j["int_bonus"]         = p.int_bonus;
	j["wis_bonus"]         = p.wis_bonus;
	j["cha_bonus"]         = p.cha_bonus;
	j["is_set_mode"]       = p.is_set_mode;
	j["nutrition_value"]   = p.nutrition_value;
	j["gold_amount"]       = p.gold_amount;
	j["ac_bonus"]          = p.ac_bonus;
	j["ranged"]            = p.ranged;
	j["hand_requirement"]  = encode_hand_requirement(p.hand_requirement);
	j["weapon_size"]       = encode_weapon_size(p.weapon_size);
	j["consumable_effect"] = encode_consumable_effect(p.consumable_effect);
	j["consumable_buff"]   = encode_buff_type(p.consumable_buff_type);
	j["target_mode"]       = encode_target_mode(p.target_mode);
	j["scroll_animation"]  = encode_scroll_animation(p.scroll_animation);
	return j;
}

ItemEntry parse_item_entry(const std::string& key, const nlohmann::json& j)
{
	ItemEntry e;
	e.name     = j.value("name", key);
	e.category = j.value("category", std::string{});
	ItemParams& p = e.params;
	p.color             = j.value("color", 0);
	p.itemClass         = parse_item_class(j.value("item_class", std::string{ "unknown" }));
	p.value             = j.value("value", 0);
	p.pickable_type     = parse_pickable_type(j.value("pickable_type", std::string{ "weapon" }));
	p.base_weight       = j.value("base_weight", 0);
	p.level_minimum     = j.value("level_minimum", 1);
	p.level_maximum     = j.value("level_maximum", 0);
	p.level_scaling     = j.value("level_scaling", 0.0f);
	p.consumable_amount = j.value("consumable_amount", 0);
	p.range             = j.value("range", 0);
	p.damage            = j.value("damage", 0);
	p.confuse_turns     = j.value("confuse_turns", 0);
	p.duration          = j.value("duration", 0);
	p.effect            = parse_magical_effect(j.value("effect", std::string{ "none" }));
	p.effect_bonus      = j.value("effect_bonus", 0);
	p.str_bonus         = j.value("str_bonus", 0);
	p.dex_bonus         = j.value("dex_bonus", 0);
	p.con_bonus         = j.value("con_bonus", 0);
	p.int_bonus         = j.value("int_bonus", 0);
	p.wis_bonus         = j.value("wis_bonus", 0);
	p.cha_bonus         = j.value("cha_bonus", 0);
	p.is_set_mode       = j.value("is_set_mode", false);
	p.nutrition_value   = j.value("nutrition_value", 0);
	p.gold_amount       = j.value("gold_amount", 0);
	p.ac_bonus          = j.value("ac_bonus", 0);
	p.ranged            = j.value("ranged", false);
	p.hand_requirement  = parse_hand_requirement(j.value("hand_requirement", std::string{ "one_handed" }));
	p.weapon_size       = parse_weapon_size(j.value("weapon_size", std::string{ "medium" }));
	p.consumable_effect = parse_consumable_effect(j.value("consumable_effect", std::string{ "none" }));
	p.consumable_buff_type = parse_buff_type(j.value("consumable_buff", std::string{ "none" }));
	p.target_mode       = parse_target_mode(j.value("target_mode", std::string{ "auto_nearest" }));
	p.scroll_animation  = parse_scroll_animation(j.value("scroll_animation", std::string{ "none" }));
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
	item.str_bonus   = p.str_bonus;
	item.dex_bonus   = p.dex_bonus;
	item.con_bonus   = p.con_bonus;
	item.int_bonus   = p.int_bonus;
	item.wis_bonus   = p.wis_bonus;
	item.cha_bonus   = p.cha_bonus;
	item.effect      = p.effect;
	item.bonus       = p.effect_bonus;
	item.is_set_mode = p.is_set_mode;
	return item;
}

ItemBehavior create_behavior(const ItemParams& params)
{
	switch (params.pickable_type)
	{
	case PickableType::CONSUMABLE:
		return Consumable{
			params.consumable_effect,
			params.consumable_amount,
			params.duration,
			params.consumable_buff_type,
			params.is_set_mode
		};

	case PickableType::TARGETED_SCROLL:
		return TargetedScroll{
			params.target_mode,
			params.scroll_animation,
			params.range,
			params.damage,
			params.confuse_turns,
			params.consumable_buff_type,
			params.duration
		};

	case PickableType::TELEPORTER:
		return Teleporter{};

	case PickableType::WEAPON:
		return Weapon{ params.ranged, params.hand_requirement, params.weapon_size };

	case PickableType::SHIELD:
		return Shield{};

	case PickableType::ARMOR:
		return Armor{ params.ac_bonus };

	case PickableType::QUEST_ITEM:
		return Amulet{};

	case PickableType::MAGICAL_HELM:
		return MagicalHelm{ params.effect, params.effect_bonus };

	case PickableType::MAGICAL_RING:
		return MagicalRing{ params.effect, params.effect_bonus };

	case PickableType::JEWELRY_AMULET:
		return create_stat_behavior<JewelryAmulet>(params);

	case PickableType::GAUNTLETS:
		return create_stat_behavior<Gauntlets>(params);

	case PickableType::GIRDLE:
		return create_stat_behavior<Girdle>(params);

	case PickableType::FOOD:
		return Food{ params.nutrition_value };

	case PickableType::GOLD_COIN:
		return Gold{ 0 };

	default:
		throw std::runtime_error(
			std::format("ItemCreator: unhandled pickable_type {}",
				static_cast<int>(params.pickable_type)));
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
	item->item_key = std::string{ key };
	item->itemClass = p.itemClass;
	item->set_value(p.value);
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
		return;

	nlohmann::json root = nlohmann::json::parse(f);
	s_registry.clear();
	s_builtin_keys.clear();

	for (const auto& [key, val] : root.items())
	{
		s_registry[key] = parse_item_entry(key, val);
		patch_views(s_registry[key]); // patch after stable insertion into map
		s_builtin_keys.insert(key);
	}
}

void ItemCreator::save(std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::filesystem::create_directories(resolved.parent_path());

	nlohmann::json root = nlohmann::json::object();
	for (const auto& [key, entry] : s_registry)
		root[key] = encode_item_entry(entry);

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
	keys.reserve(s_registry.size());
	for (const auto& [key, _] : s_registry)
		keys.push_back(key);
	return keys;
}

const ItemParams& ItemCreator::get_params(std::string_view key)
{
	auto it = s_registry.find(std::string{ key });
	if (it == s_registry.end())
	{
		throw std::out_of_range(
			std::format("ItemCreator::get_params -- unknown key '{}'", key));
	}
	return it->second.params;
}

void ItemCreator::set_params(std::string_view key, const ItemParams& params)
{
	auto it = s_registry.find(std::string{ key });
	if (it == s_registry.end())
	{
		throw std::out_of_range(
			std::format("ItemCreator::set_params -- unknown key '{}'", key));
	}
	it->second.params = params;
	patch_views(it->second);
}

std::unique_ptr<Item> ItemCreator::create(std::string_view key, Vector2D pos, ContentRegistry& registry)
{
	auto it = s_registry.find(std::string{ key });
	if (it == s_registry.end())
	{
		throw std::out_of_range(
			std::format("ItemCreator::create -- unknown key '{}'", key));
	}
	return make_item(key, it->second, pos, registry);
}

std::unique_ptr<Item> ItemCreator::create_with_gold_amount(Vector2D pos, int goldAmount, ContentRegistry& registry)
{
	auto it = s_registry.find("gold_coin");
	if (it == s_registry.end())
	{
		throw std::runtime_error(
			"ItemCreator::create_with_gold_amount -- 'gold_coin' not in registry");
	}
	const ItemParams& p = it->second.params;
	TileRef tile = registry.get_tile("gold_coin");
	auto item = std::make_unique<Item>(
		pos,
		ActorData{ tile, std::string{ p.name }, p.color });
	item->behavior = Gold{ goldAmount };
	item->item_key = "gold_coin";
	item->itemClass = p.itemClass;
	item->set_value(goldAmount);
	return item;
}

std::unique_ptr<Item> ItemCreator::create_with_enhancement(
	std::string_view key,
	Vector2D pos,
	PrefixType prefix,
	SuffixType suffix,
	ContentRegistry& registry)
{
	auto item = create(key, pos, registry);
	item->apply_enhancement(ItemEnhancement(prefix, suffix));
	return item;
}

std::unique_ptr<Item> ItemCreator::create_gold_pile(Vector2D pos, GameContext& ctx)
{
	const int goldAmount = ctx.dice->roll(5, 20);
	return create_with_gold_amount(pos, goldAmount, *ctx.content_registry);
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

	for (const auto& [key, entry] : s_registry)
	{
		const ItemParams& p = entry.params;
		if (p.category != category || p.base_weight <= 0)
			continue;
		if (dungeonLevel < p.level_minimum)
			continue;
		if (p.level_maximum > 0 && dungeonLevel > p.level_maximum)
			continue;

		const float levelFactor = 1.0f + (p.level_scaling * static_cast<float>(dungeonLevel - 1));
		const int weight = std::max(1, static_cast<int>(p.base_weight * levelFactor));
		candidates.push_back({ key, weight });
		totalWeight += weight;
	}

	if (candidates.empty())
		return nullptr;

	int roll = ctx.dice->roll(1, totalWeight);
	for (const auto& [key, weight] : candidates)
	{
		roll -= weight;
		if (roll <= 0)
			return create(key, pos, *ctx.content_registry);
	}

	return create(candidates.back().key, pos, *ctx.content_registry);
}

std::string ItemCreator::add_custom(std::string name, std::string category, ItemParams params)
{
	const std::string key = unique_item_key(normalize_key(name.empty() ? "new_item" : name));
	ItemEntry& entry = s_registry[key];
	entry.name     = std::move(name);
	entry.category = std::move(category);
	entry.params   = params;
	patch_views(entry);
	return key;
}

void ItemCreator::remove_custom(std::string_view key)
{
	if (s_builtin_keys.contains(std::string{ key }))
	{
		throw std::logic_error(
			std::format("ItemCreator::remove_custom -- '{}' is a built-in item", key));
	}
	if (!s_registry.erase(std::string{ key }))
	{
		throw std::out_of_range(
			std::format("ItemCreator::remove_custom -- unknown key '{}'", key));
	}
}

void ItemCreator::set_name_category(std::string_view key, std::string name, std::string category)
{
	auto it = s_registry.find(std::string{ key });
	if (it == s_registry.end())
	{
		throw std::out_of_range(
			std::format("ItemCreator::set_name_category -- unknown key '{}'", key));
	}
	it->second.name = std::move(name);
	it->second.category = std::move(category);
	patch_views(it->second);
}

bool ItemCreator::is_builtin_key(std::string_view key)
{
	return s_builtin_keys.contains(std::string{ key });
}

void ItemCreator::load_enhanced_rules(std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::ifstream f(resolved);
	if (!f.is_open())
		return;

	nlohmann::json root = nlohmann::json::parse(f);
	s_enhanced_rules.clear();

	for (const auto& entry : root)
	{
		EnhancedItemSpawnRule rule;

		std::string cat = entry.at("enhancement_category").get<std::string>();
		rule.enhancement_category = (cat == "weapon")
			? EnhancedItemCategory::WEAPON
			: EnhancedItemCategory::ARMOR;

		rule.base_weight   = entry.at("base_weight").get<int>();
		rule.level_minimum = entry.at("level_minimum").get<int>();
		rule.level_maximum = entry.value("level_maximum", 0);
		rule.level_scaling = entry.at("level_scaling").get<float>();
		rule.category      = entry.at("category").get<std::string>();

		for (const auto& key : entry.at("item_pool"))
			rule.item_pool.push_back(key.get<std::string>());

		s_enhanced_rules.push_back(std::move(rule));
	}
}

std::span<const EnhancedItemSpawnRule> ItemCreator::get_enhanced_rules()
{
	return s_enhanced_rules;
}
