#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "EquipmentSlot.h"
#include "MonsterRegistry.h"
#include "Paths.h"

namespace
{

// ---------------------------------------------------------------------------
// Key <-> MonsterId mappings
// ---------------------------------------------------------------------------
std::string_view monster_key(MonsterId id)
{
	switch (id)
	{
	case MonsterId::GOBLIN:
	{
		return "goblin";
	}
	case MonsterId::ORC:
	{
		return "orc";
	}
	case MonsterId::TROLL:
	{
		return "troll";
	}
	case MonsterId::DRAGON:
	{
		return "dragon";
	}
	case MonsterId::ARCHER:
	{
		return "archer";
	}
	case MonsterId::MAGE:
	{
		return "mage";
	}
	case MonsterId::WOLF:
	{
		return "wolf";
	}
	case MonsterId::FIRE_WOLF:
	{
		return "fire_wolf";
	}
	case MonsterId::ICE_WOLF:
	{
		return "ice_wolf";
	}
	case MonsterId::BAT:
	{
		return "bat";
	}
	case MonsterId::KOBOLD:
	{
		return "kobold";
	}
	case MonsterId::MIMIC:
	{
		return "mimic";
	}
	case MonsterId::SHOPKEEPER:
	{
		return "shopkeeper";
	}
	case MonsterId::SPIDER_SMALL:
	{
		return "spider_small";
	}
	case MonsterId::SPIDER_GIANT:
	{
		return "spider_giant";
	}
	case MonsterId::SPIDER_WEAVER:
	{
		return "spider_weaver";
	}
	}
	return "unknown";
}

// Standard monsters in declaration order (used for load/save iteration).
constexpr MonsterId STANDARD_IDS[] = {
	MonsterId::GOBLIN,
	MonsterId::ORC,
	MonsterId::TROLL,
	MonsterId::DRAGON,
	MonsterId::ARCHER,
	MonsterId::MAGE,
	MonsterId::WOLF,
	MonsterId::FIRE_WOLF,
	MonsterId::ICE_WOLF,
	MonsterId::BAT,
	MonsterId::KOBOLD,
};

constexpr MonsterId CLASS_IDS[] = {
	MonsterId::MIMIC,
	MonsterId::SHOPKEEPER,
	MonsterId::SPIDER_SMALL,
	MonsterId::SPIDER_GIANT,
	MonsterId::SPIDER_WEAVER,
};

// ---------------------------------------------------------------------------
// JSON helpers
// ---------------------------------------------------------------------------
TileRef parse_tile(const nlohmann::json& record)
{
	return TileRef{
		static_cast<TileSheet>(record.at("sheet").get<int>()),
		record.at("col").get<int>(),
		record.at("row").get<int>()
	};
}

nlohmann::json encode_tile(TileRef tile)
{
	return nlohmann::json{
		{ "sheet", static_cast<int>(tile.sheet) },
		{ "col", tile.col },
		{ "row", tile.row }
	};
}

DiceExpr parse_dice(const nlohmann::json& record)
{
	return DiceExpr{
		record.at("num").get<int>(),
		record.at("sides").get<int>(),
		record.at("bonus").get<int>()
	};
}

nlohmann::json encode_dice(const DiceExpr& dice)
{
	return nlohmann::json{
		{ "num", dice.num },
		{ "sides", dice.sides },
		{ "bonus", dice.bonus }
	};
}

// The dice are the record; the range is theirs to derive, so a file cannot
// carry a minimum its dice never roll.
DamageInfo parse_damage(const nlohmann::json& record)
{
	return DamageInfo{
		record.at("display").get<std::string>(),
		parse_damage_type(record.at("type").get<std::string>())
	};
}

nlohmann::json encode_damage(const DamageInfo& damage)
{
	return nlohmann::json{
		{ "display", damage.displayRoll },
		{ "type", damage_type_name(damage.damageType) }
	};
}

// Reads the slots a creature starts filled. An unknown slot name or a missing
// field throws, so a typo fails at load rather than leaving a monster unarmed.
std::vector<MonsterParams::StartingItem> parse_equipment(const nlohmann::json& entries)
{
	std::vector<MonsterParams::StartingItem> equipment;
	equipment.reserve(entries.size());

	for (const nlohmann::json& entry : entries)
	{
		MonsterParams::StartingItem carried;
		carried.itemKey = entry.at("item").get<std::string>();
		carried.slot = parse_equipment_slot(entry.at("slot").get<std::string>());
		equipment.push_back(std::move(carried));
	}

	return equipment;
}

nlohmann::json encode_equipment(const std::vector<MonsterParams::StartingItem>& equipment)
{
	nlohmann::json entries = nlohmann::json::array();

	for (const MonsterParams::StartingItem& carried : equipment)
	{
		entries.push_back(nlohmann::json{
			{ "item", carried.itemKey },
			{ "slot", encode_equipment_slot(carried.slot) } });
	}

	return entries;
}

MonsterParams parse_full_params(const nlohmann::json& entry)
{
	MonsterParams params;
	params.symbol = parse_tile(entry.at("tile"));
	params.color = entry.at("color").get<int>();
	params.corpseName = entry.at("corpse").get<std::string>();
	params.hpDice = parse_dice(entry.at("hp"));
	params.thaco = entry.at("thaco").get<int>();
	params.ac = entry.at("ac").get<int>();
	params.xp = entry.at("xp").get<int>();
	params.dr = entry.at("dr").get<int>();
	params.morale = entry.at("morale").get<int>();
	// Optional by design: entries authored before turning existed are living.
	params.undead = entry.value("undead", false);
	// Optional by design: entries authored before alignment existed default to
	// true neutral rather than failing to load.
	params.ethics = parse_ethics(entry.value("ethics", std::string{ encode_ethics(Ethics::NEUTRAL) }));
	params.morality = parse_morality(entry.value("morality", std::string{ encode_morality(Morality::NEUTRAL) }));
	params.corpseWeight = entry.value("corpse_weight", 50);
	params.strDice = parse_dice(entry.at("str"));
	params.dexDice = parse_dice(entry.at("dex"));
	params.conDice = parse_dice(entry.at("con"));
	params.intDice = parse_dice(entry.at("int"));
	params.wisDice = parse_dice(entry.at("wis"));
	params.chaDice = parse_dice(entry.at("cha"));
	params.naturalAttack = entry.at("natural_attack").get<std::string>();
	params.equipment = parse_equipment(entry.at("equipment"));
	params.bodyPlanName = entry.at("body").get<std::string>();
	params.damage = parse_damage(entry.at("damage"));
	params.aiType = (entry.at("ai").get<std::string>() == "ranged")
		? MonsterAiType::RANGED
		: MonsterAiType::MELEE;
	params.canSwim = entry.at("can_swim").get<bool>();
	params.baseWeight = entry.at("weight").get<int>();
	params.levelMinimum = entry.at("depth_min").get<int>();
	params.levelMaximum = entry.at("depth_max").get<int>();
	params.levelScaling = entry.at("depth_scale").get<float>();
	// Optional display name - overrides the key-derived fallback.
	if (entry.contains("name"))
	{
		params.name = entry.at("name").get<std::string>();
	}
	return params;
}

nlohmann::json encode_full_params(const MonsterParams& params)
{
	return nlohmann::json{
		{ "tile", encode_tile(params.symbol) },
		{ "color", params.color },
		{ "name", params.name },
		{ "corpse", params.corpseName },
		{ "hp", encode_dice(params.hpDice) },
		{ "thaco", params.thaco },
		{ "ac", params.ac },
		{ "xp", params.xp },
		{ "dr", params.dr },
		{ "morale", params.morale },
		{ "undead", params.undead },
		{ "ethics", encode_ethics(params.ethics) },
		{ "morality", encode_morality(params.morality) },
		{ "corpse_weight", params.corpseWeight },
		{ "str", encode_dice(params.strDice) },
		{ "dex", encode_dice(params.dexDice) },
		{ "con", encode_dice(params.conDice) },
		{ "int", encode_dice(params.intDice) },
		{ "wis", encode_dice(params.wisDice) },
		{ "cha", encode_dice(params.chaDice) },
		{ "natural_attack", params.naturalAttack },
		{ "equipment", encode_equipment(params.equipment) },
		{ "body", params.bodyPlanName },
		{ "damage", encode_damage(params.damage) },
		{ "ai", params.aiType == MonsterAiType::RANGED ? "ranged" : "melee" },
		{ "can_swim", params.canSwim },
		{ "weight", params.baseWeight },
		{ "depth_min", params.levelMinimum },
		{ "depth_max", params.levelMaximum },
		{ "depth_scale", params.levelScaling }
	};
}

// ---------------------------------------------------------------------------
// String-key helpers
// ---------------------------------------------------------------------------
std::optional<MonsterId> key_to_standard_id(std::string_view key)
{
	for (MonsterId id : STANDARD_IDS)
	{
		if (monster_key(id) == key)
		{
			return id;
		}
	}

	return std::nullopt;
}

std::optional<MonsterId> key_to_class_id(std::string_view key)
{
	for (MonsterId id : CLASS_IDS)
	{
		if (monster_key(id) == key)
		{
			return id;
		}
	}

	return std::nullopt;
}

std::string normalize_key(std::string_view name)
{
	std::string key;
	key.reserve(name.size());
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
}

} // anonymous namespace

// ---------------------------------------------------------------------------

void MonsterRegistry::load(std::string_view path)
{
	const auto resolved = Paths::resolve(path);
	std::ifstream file(resolved);
	if (!file.is_open())
	{
		throw std::runtime_error(
			std::format("MonsterRegistry::load -- cannot open '{}'", resolved.string()));
	}

	const nlohmann::json root = nlohmann::json::parse(file);

	standardMonsters.clear();
	classTiles.clear();
	customMonsters.clear();
	classParams.clear();

	// Collect known builtin keys for custom detection
	std::set<std::string> knownKeys;
	for (MonsterId id : STANDARD_IDS)
	{
		knownKeys.insert(std::string{ monster_key(id) });
	}

	for (MonsterId id : CLASS_IDS)
	{
		knownKeys.insert(std::string{ monster_key(id) });
	}

	// Standard monsters: entry has "hp" field.
	for (MonsterId id : STANDARD_IDS)
	{
		const std::string key{ monster_key(id) };
		if (!root.contains(key))
		{
			throw std::runtime_error(
				std::format("MonsterRegistry::load -- missing entry '{}'", key));
		}
		MonsterParams& params = standardMonsters[id];
		params = parse_full_params(root.at(key));
		const std::string deadPrefix = "dead ";
		if (params.corpseName.starts_with(deadPrefix))
		{
			params.name = params.corpseName.substr(deadPrefix.size());
		}
		else
		{
			params.name = key;
		}
	}

	// Class-based monsters: entry only has tile (and optional color).
	for (MonsterId id : CLASS_IDS)
	{
		const std::string key{ monster_key(id) };
		if (root.contains(key))
		{
			classTiles[id] = parse_tile(root.at(key).at("tile"));
		}
		MonsterParams& params = classParams[id];
		params.name = key;
		if (classTiles.contains(id))
		{
			params.symbol = classTiles.at(id);
		}
	}

	// Custom monsters: any remaining JSON entry with an "hp" field
	for (const auto& [key, entry] : root.items())
	{
		if (knownKeys.contains(key))
		{
			continue;
		}

		if (!entry.contains("hp"))
		{
			continue;
		}

		MonsterParams params = parse_full_params(entry);
		if (params.name.empty())
		{
			params.name = key;
		}
		customMonsters[key] = std::move(params);
	}
}

void MonsterRegistry::save(std::string_view path) const
{
	const auto resolved = Paths::resolve(path);
	std::filesystem::create_directories(resolved.parent_path());

	nlohmann::json root = nlohmann::json::object();

	for (MonsterId id : STANDARD_IDS)
	{
		if (standardMonsters.contains(id))
		{
			root[std::string{ monster_key(id) }] = encode_full_params(standardMonsters.at(id));
		}
	}

	for (MonsterId id : CLASS_IDS)
	{
		nlohmann::json entry = nlohmann::json::object();
		if (classTiles.contains(id))
		{
			entry["tile"] = encode_tile(classTiles.at(id));
		}

		if (standardMonsters.contains(id))
		{
			entry["color"] = standardMonsters.at(id).color;
		}
		root[std::string{ monster_key(id) }] = entry;
	}

	for (const auto& [key, params] : customMonsters)
	{
		root[key] = encode_full_params(params);
	}

	std::ofstream file(resolved);
	if (!file.is_open())
	{
		throw std::runtime_error(
			std::format("MonsterRegistry::save -- cannot open '{}' for writing", resolved.string()));
	}
	file << root.dump(4);
	if (file.fail())
	{
		throw std::runtime_error(
			std::format("MonsterRegistry::save -- write failed for '{}'", resolved.string()));
	}
}

const std::unordered_map<MonsterId, MonsterParams>& MonsterRegistry::get_standard_monsters() const
{
	return standardMonsters;
}

TileRef MonsterRegistry::get_tile(MonsterId id) const
{
	if (standardMonsters.contains(id))
	{
		return standardMonsters.at(id).symbol;
	}

	if (classTiles.contains(id))
	{
		return classTiles.at(id);
	}

	return TileRef{};
}

void MonsterRegistry::set_tile(MonsterId id, TileRef tile)
{
	if (standardMonsters.contains(id))
	{
		standardMonsters.at(id).symbol = tile;
	}
	else
	{
		classTiles[id] = tile;
	}
}

std::vector<std::string> MonsterRegistry::get_all_keys() const
{
	std::vector<std::string> keys;
	keys.reserve(std::size(STANDARD_IDS) + customMonsters.size() + std::size(CLASS_IDS));
	for (MonsterId id : STANDARD_IDS)
	{
		keys.push_back(std::string{ monster_key(id) });
	}
	for (const auto& [key, _] : customMonsters)
	{
		keys.push_back(key);
	}
	for (MonsterId id : CLASS_IDS)
	{
		keys.push_back(std::string{ monster_key(id) });
	}

	return keys;
}

const MonsterParams& MonsterRegistry::get_params(std::string_view key) const
{
	if (const std::optional<MonsterId> id = key_to_standard_id(key))
	{
		if (!standardMonsters.contains(*id))
		{
			throw std::out_of_range(std::format("MonsterRegistry::get_params -- '{}' not in registry", key));
		}

		return standardMonsters.at(*id);
	}

	if (customMonsters.contains(key))
	{
		return customMonsters.at(std::string{ key });
	}

	if (const std::optional<MonsterId> id = key_to_class_id(key))
	{
		if (!classParams.contains(*id))
		{
			throw std::out_of_range(std::format("MonsterRegistry::get_params -- class entry '{}' not initialised", key));
		}

		return classParams.at(*id);
	}

	throw std::out_of_range(std::format("MonsterRegistry::get_params -- unknown key '{}'", key));
}

void MonsterRegistry::set_params(std::string_view key, const MonsterParams& params)
{
	if (const std::optional<MonsterId> id = key_to_standard_id(key))
	{
		standardMonsters[*id] = params;
		return;
	}

	if (const std::optional<MonsterId> id = key_to_class_id(key))
	{
		classTiles[*id] = params.symbol;
		classParams[*id] = params;
		return;
	}

	if (customMonsters.contains(key))
	{
		customMonsters.at(std::string{ key }) = params;
		return;
	}

	throw std::out_of_range(std::format("MonsterRegistry::set_params -- unknown key '{}'", key));
}

TileRef MonsterRegistry::get_tile(std::string_view key) const
{
	if (const std::optional<MonsterId> id = key_to_standard_id(key))
	{
		return get_tile(*id);
	}

	if (const std::optional<MonsterId> id = key_to_class_id(key))
	{
		return get_tile(*id);
	}

	if (customMonsters.contains(key))
	{
		return customMonsters.at(std::string{ key }).symbol;
	}

	return TileRef{};
}

void MonsterRegistry::set_tile(std::string_view key, TileRef tile)
{
	if (const std::optional<MonsterId> id = key_to_standard_id(key))
	{
		set_tile(*id, tile);
		return;
	}

	if (const std::optional<MonsterId> id = key_to_class_id(key))
	{
		set_tile(*id, tile);
		return;
	}

	if (customMonsters.contains(key))
	{
		customMonsters.at(std::string{ key }).symbol = tile;
		return;
	}

	throw std::out_of_range(std::format("MonsterRegistry::set_tile -- unknown key '{}'", key));
}

std::string MonsterRegistry::add_custom(MonsterParams params)
{
	auto is_taken = [this](std::string_view key) -> bool
	{
		return key_to_standard_id(key).has_value() || key_to_class_id(key).has_value() || customMonsters.contains(key);
	};

	const std::string base = normalize_key(params.name.empty() ? "new_monster" : params.name);
	std::string key = base;
	if (is_taken(key))
	{
		for (int suffix = 2;; ++suffix)
		{
			key = std::format("{}_{}", base, suffix);
			if (!is_taken(key))
			{
				break;
			}
		}
	}
	customMonsters[key] = std::move(params);

	return key;
}

void MonsterRegistry::remove_custom(std::string_view key)
{
	if (key_to_standard_id(key) || key_to_class_id(key))
	{
		throw std::invalid_argument(
			std::format("MonsterRegistry::remove_custom -- '{}' is a built-in monster and cannot be removed", key));
	}

	if (!customMonsters.erase(std::string{ key }))
	{
		throw std::out_of_range(
			std::format("MonsterRegistry::remove_custom -- unknown key '{}'", key));
	}
}

bool MonsterRegistry::is_builtin(std::string_view key) const
{
	return key_to_standard_id(key).has_value();
}

bool MonsterRegistry::is_class_key(std::string_view key) const
{
	return key_to_class_id(key).has_value();
}
