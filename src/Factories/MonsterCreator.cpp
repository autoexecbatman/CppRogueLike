// file: MonsterCreator.cpp
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "../Actor/MonsterAttacker.h"
#include "../Actor/Creature.h"
#include "../Ai/AiMonster.h"
#include "../Ai/AiMonsterRanged.h"
#include "../Colors/Colors.h"
#include "../Combat/DamageInfo.h"
#include "../Combat/ExperienceReward.h"
#include "../Combat/HealthPool.h"
#include "../Core/GameContext.h"
#include "../Core/Paths.h"
#include "../Random/RandomDice.h"
#include "../Utils/Vector2D.h"
#include "MonsterCreator.h"

namespace
{

// ---------------------------------------------------------------------------
// Module-level storage
// Standard monsters: full params (stats + tile).
// Class-based monsters (mimic, shopkeeper, spiders): tile only.
// Custom monsters: user-created, persisted to JSON alongside builtins.
// ---------------------------------------------------------------------------
std::unordered_map<MonsterId, MonsterParams> registry;
std::unordered_map<MonsterId, TileRef> s_class_tiles;
std::unordered_map<MonsterId, MonsterParams> s_class_params; // minimal params for class-based
std::map<std::string, MonsterParams> s_custom; // user-created monsters

// ---------------------------------------------------------------------------
// Key <-> MonsterId mappings
// ---------------------------------------------------------------------------
std::string_view monster_key(MonsterId id)
{
	switch (id)
	{
	case MonsterId::GOBLIN:
		return "goblin";
	case MonsterId::ORC:
		return "orc";
	case MonsterId::TROLL:
		return "troll";
	case MonsterId::DRAGON:
		return "dragon";
	case MonsterId::ARCHER:
		return "archer";
	case MonsterId::MAGE:
		return "mage";
	case MonsterId::WOLF:
		return "wolf";
	case MonsterId::FIRE_WOLF:
		return "fire_wolf";
	case MonsterId::ICE_WOLF:
		return "ice_wolf";
	case MonsterId::BAT:
		return "bat";
	case MonsterId::KOBOLD:
		return "kobold";
	case MonsterId::MIMIC:
		return "mimic";
	case MonsterId::SHOPKEEPER:
		return "shopkeeper";
	case MonsterId::SPIDER_SMALL:
		return "spider_small";
	case MonsterId::SPIDER_GIANT:
		return "spider_giant";
	case MonsterId::SPIDER_WEAVER:
		return "spider_weaver";
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
TileRef parse_tile(const nlohmann::json& j)
{
	return TileRef{
		static_cast<TileSheet>(j.at("sheet").get<int>()),
		j.at("col").get<int>(),
		j.at("row").get<int>()
	};
}

nlohmann::json encode_tile(TileRef t)
{
	return nlohmann::json{
		{ "sheet", static_cast<int>(t.sheet) },
		{ "col", t.col },
		{ "row", t.row }
	};
}

DiceExpr parse_dice(const nlohmann::json& j)
{
	return DiceExpr{
		j.at("num").get<int>(),
		j.at("sides").get<int>(),
		j.at("bonus").get<int>()
	};
}

nlohmann::json encode_dice(const DiceExpr& d)
{
	return nlohmann::json{
		{ "num", d.num },
		{ "sides", d.sides },
		{ "bonus", d.bonus }
	};
}

DamageInfo parse_damage(const nlohmann::json& j)
{
	return DamageInfo{
		j.at("min").get<int>(),
		j.at("max").get<int>(),
		j.at("display").get<std::string>(),
		static_cast<DamageType>(j.at("type").get<int>())
	};
}

nlohmann::json encode_damage(const DamageInfo& d)
{
	return nlohmann::json{
		{ "min", d.minDamage },
		{ "max", d.maxDamage },
		{ "display", d.displayRoll },
		{ "type", static_cast<int>(d.damageType) }
	};
}

MonsterParams parse_full_params(const nlohmann::json& entry)
{
	MonsterParams p;
	p.symbol = parse_tile(entry.at("tile"));
	p.color = entry.at("color").get<int>();
	p.corpseName = entry.at("corpse").get<std::string>();
	p.hpDice = parse_dice(entry.at("hp"));
	p.thaco = entry.at("thaco").get<int>();
	p.ac = entry.at("ac").get<int>();
	p.xp = entry.at("xp").get<int>();
	p.dr = entry.at("dr").get<int>();
	p.morale = entry.at("morale").get<int>();
	p.strDice = parse_dice(entry.at("str"));
	p.dexDice = parse_dice(entry.at("dex"));
	p.conDice = parse_dice(entry.at("con"));
	p.intDice = parse_dice(entry.at("int"));
	p.wisDice = parse_dice(entry.at("wis"));
	p.chaDice = parse_dice(entry.at("cha"));
	p.weaponName = entry.at("weapon").get<std::string>();
	p.damage = parse_damage(entry.at("damage"));
	p.aiType = (entry.at("ai").get<std::string>() == "ranged")
		? MonsterAiType::RANGED
		: MonsterAiType::MELEE;
	p.canSwim = entry.at("can_swim").get<bool>();
	p.baseWeight = entry.at("weight").get<int>();
	p.levelMinimum = entry.at("depth_min").get<int>();
	p.levelMaximum = entry.at("depth_max").get<int>();
	p.levelScaling = entry.at("depth_scale").get<float>();
	// Optional display name — overrides the key-derived fallback.
	if (entry.contains("name"))
	{
		p.name = entry.at("name").get<std::string>();
	}
	return p;
}

nlohmann::json encode_full_params(const MonsterParams& p)
{
	return nlohmann::json{
		{ "tile", encode_tile(p.symbol) },
		{ "color", p.color },
		{ "corpse", p.corpseName },
		{ "hp", encode_dice(p.hpDice) },
		{ "thaco", p.thaco },
		{ "ac", p.ac },
		{ "xp", p.xp },
		{ "dr", p.dr },
		{ "morale", p.morale },
		{ "str", encode_dice(p.strDice) },
		{ "dex", encode_dice(p.dexDice) },
		{ "con", encode_dice(p.conDice) },
		{ "int", encode_dice(p.intDice) },
		{ "wis", encode_dice(p.wisDice) },
		{ "cha", encode_dice(p.chaDice) },
		{ "weapon", p.weaponName },
		{ "damage", encode_damage(p.damage) },
		{ "ai", p.aiType == MonsterAiType::RANGED ? "ranged" : "melee" },
		{ "can_swim", p.canSwim },
		{ "weight", p.baseWeight },
		{ "depth_min", p.levelMinimum },
		{ "depth_max", p.levelMaximum },
		{ "depth_scale", p.levelScaling }
	};
}

int roll_dice(RandomDice* dice, const DiceExpr& expr)
{
	if (expr.num == 0)
	{
		return 0;
	}

	int total = 0;
	for (int i = 0; i < expr.num; ++i)
	{
		total += dice->roll(1, expr.sides);
	}

	return total + expr.bonus;
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
	for (char c : name)
	{
		if (std::isspace(static_cast<unsigned char>(c)))
		{
			key += '_';
		}
		else
		{
			key += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
	}

	return key;
}

bool has_any_key(std::string_view key)
{
	for (MonsterId id : STANDARD_IDS)
	{
		if (monster_key(id) == key)
		{
			return true;
		}
	}

	for (MonsterId id : CLASS_IDS)
	{
		if (monster_key(id) == key)
		{
			return true;
		}
	}

	return s_custom.contains(std::string{ key });
}

std::string unique_key(std::string base)
{
	if (!has_any_key(base))
	{
		return base;
	}

	for (int n = 2;; ++n)
	{
		std::string candidate = std::format("{}_{}", base, n);
		if (!has_any_key(candidate))
		{
			return candidate;
		}
	}
}

} // anonymous namespace

// ---------------------------------------------------------------------------

void MonsterCreator::load(std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::ifstream f(resolved);
	if (!f.is_open())
	{
		throw std::runtime_error(
			std::format("MonsterCreator::load -- cannot open '{}'", resolved.string()));
	}

	nlohmann::json root = nlohmann::json::parse(f);

	registry.clear();
	s_class_tiles.clear();
	s_custom.clear();
	s_class_params.clear();

	// Collect known builtin keys for custom detection
	std::set<std::string> known_keys;
	for (MonsterId id : STANDARD_IDS)
	{
		known_keys.insert(std::string{ monster_key(id) });
	}

	for (MonsterId id : CLASS_IDS)
	{
		known_keys.insert(std::string{ monster_key(id) });
	}

	// Standard monsters: entry has "hp" field.
	for (MonsterId id : STANDARD_IDS)
	{
		const std::string key{ monster_key(id) };
		if (!root.contains(key))
		{
			throw std::runtime_error(
				std::format("MonsterCreator::load -- missing entry '{}'", key));
		}
		registry[id] = parse_full_params(root.at(key));
		const auto& corpse = registry[id].corpseName;
		const std::string dead_prefix = "dead ";
		if (corpse.starts_with(dead_prefix))
		{
			registry[id].name = corpse.substr(dead_prefix.size());
		}
		else
		{
			registry[id].name = key;
		}
	}

	// Class-based monsters: entry only has tile (and optional color).
	for (MonsterId id : CLASS_IDS)
	{
		const std::string key{ monster_key(id) };
		if (root.contains(key))
		{
			s_class_tiles[id] = parse_tile(root.at(key).at("tile"));
		}
		MonsterParams& p = s_class_params[id];
		p.name = std::string{ monster_key(id) };
		if (s_class_tiles.contains(id))
		{
			p.symbol = s_class_tiles.at(id);
		}
	}

	// Custom monsters: any remaining JSON entry with an "hp" field
	for (const auto& [key, entry] : root.items())
	{
		if (known_keys.contains(key))
		{
			continue;
		}

		if (!entry.contains("hp"))
		{
			continue;
		}

		MonsterParams p = parse_full_params(entry);
		if (p.name.empty())
		{
			p.name = key;
		}
		s_custom[key] = std::move(p);
	}
}

void MonsterCreator::save(std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::filesystem::create_directories(resolved.parent_path());

	nlohmann::json root = nlohmann::json::object();

	for (MonsterId id : STANDARD_IDS)
	{
		if (registry.contains(id))
		{
			root[std::string{ monster_key(id) }] = encode_full_params(registry.at(id));
		}
	}

	for (MonsterId id : CLASS_IDS)
	{
		nlohmann::json entry = nlohmann::json::object();
		if (s_class_tiles.contains(id))
		{
			entry["tile"] = encode_tile(s_class_tiles.at(id));
		}

		if (registry.contains(id))
		{
			entry["color"] = registry.at(id).color;
		}
		root[std::string{ monster_key(id) }] = entry;
	}

	for (const auto& [key, params] : s_custom)
	{
		root[key] = encode_full_params(params);
	}

	std::ofstream f(resolved);
	if (!f.is_open())
	{
		throw std::runtime_error(
			std::format("MonsterCreator::save -- cannot open '{}' for writing", resolved.string()));
	}
	f << root.dump(4);
	if (f.fail())
	{
		throw std::runtime_error(
			std::format("MonsterCreator::save -- write failed for '{}'", resolved.string()));
	}
}

const std::unordered_map<MonsterId, MonsterParams>& MonsterCreator::get_registry()
{
	return registry;
}

TileRef MonsterCreator::get_tile(MonsterId id)
{
	if (registry.contains(id))
	{
		return registry.at(id).symbol;
	}

	if (s_class_tiles.contains(id))
	{
		return s_class_tiles.at(id);
	}

	return TileRef{};
}

void MonsterCreator::set_params(MonsterId id, const MonsterParams& p)
{
	if (registry.contains(id))
	{
		registry[id] = p;
	}
}

void MonsterCreator::set_tile(MonsterId id, TileRef tile)
{
	if (registry.contains(id))
	{
		registry.at(id).symbol = tile;
	}
	else
	{
		s_class_tiles[id] = tile;
	}
}

std::unique_ptr<Creature> MonsterCreator::create_from_params(
	Vector2D pos,
	const MonsterParams& params,
	GameContext& ctx)
{
	auto c = std::make_unique<Creature>(pos, ActorData{ params.symbol, params.name, params.color });

	c->set_strength(std::max(1, roll_dice(ctx.dice, params.strDice)));
	c->set_dexterity(std::max(1, roll_dice(ctx.dice, params.dexDice)));
	c->set_constitution(std::max(1, roll_dice(ctx.dice, params.conDice)));
	c->set_intelligence(std::max(1, roll_dice(ctx.dice, params.intDice)));
	c->set_wisdom(std::max(1, roll_dice(ctx.dice, params.wisDice)));
	c->set_charisma(std::max(1, roll_dice(ctx.dice, params.chaDice)));

	c->set_weapon_equipped(params.weaponName);
	c->set_morale(params.morale);
	c->set_creature_level(params.hpDice.num);

	const int hp = std::max(1, roll_dice(ctx.dice, params.hpDice));

	c->attacker = std::make_unique<MonsterAttacker>(*c, params.damage);
	c->experienceReward = std::make_unique<ExperienceReward>(params.xp);
	c->set_dr(params.dr);
	c->set_thaco(params.thaco);
	c->armorClass = std::make_unique<ArmorClass>(params.ac);
	c->healthPool = std::make_unique<HealthPool>(hp);
	c->set_last_constitution(c->get_constitution());

	if (params.aiType == MonsterAiType::RANGED)
	{
		c->ai = std::make_unique<AiMonsterRanged>();
		c->add_state(ActorState::IS_RANGED);
	}
	else
	{
		c->ai = std::make_unique<AiMonster>();
	}

	if (params.canSwim)
	{
		c->add_state(ActorState::CAN_SWIM);
	}

	assert(c->ai && "Monster requires Ai");
	assert(c->attacker && "Monster requires Attacker");

	return c;
}

std::unique_ptr<Creature> MonsterCreator::create(Vector2D pos, MonsterId id, GameContext& ctx)
{
	return create_from_params(pos, registry.at(id), ctx);
}

// ---------------------------------------------------------------------------
// String-keyed API (editor / factory)
// ---------------------------------------------------------------------------

std::vector<std::string> MonsterCreator::get_all_keys()
{
	std::vector<std::string> keys;
	keys.reserve(std::size(STANDARD_IDS) + s_custom.size() + std::size(CLASS_IDS));
	for (MonsterId id : STANDARD_IDS)
	{
		keys.push_back(std::string{ monster_key(id) });
	}
	for (const auto& [key, _] : s_custom)
	{
		keys.push_back(key);
	}
	for (MonsterId id : CLASS_IDS)
	{
		keys.push_back(std::string{ monster_key(id) });
	}

	return keys;
}

const MonsterParams& MonsterCreator::get_params(std::string_view key)
{
	if (auto id = key_to_standard_id(key))
	{
		if (!registry.contains(*id))
		{
			throw std::out_of_range(std::format("MonsterCreator::get_params -- '{}' not in registry", key));
		}

		return registry.at(*id);
	}

	if (s_custom.contains(std::string{ key }))
	{
		return s_custom.at(std::string{ key });
	}

	if (auto id = key_to_class_id(key))
	{
		if (!s_class_params.contains(*id))
		{
			throw std::out_of_range(std::format("MonsterCreator::get_params -- class entry '{}' not initialised", key));
		}

		return s_class_params.at(*id);
	}

	throw std::out_of_range(std::format("MonsterCreator::get_params -- unknown key '{}'", key));
}

void MonsterCreator::set_params(std::string_view key, const MonsterParams& p)
{
	if (auto id = key_to_standard_id(key))
	{
		registry[*id] = p;
		return;
	}

	if (auto id = key_to_class_id(key))
	{
		s_class_tiles[*id] = p.symbol;
		s_class_params[*id] = p;
		return;
	}

	if (s_custom.contains(std::string{ key }))
	{
		s_custom[std::string{ key }] = p;
		return;
	}

	throw std::out_of_range(std::format("MonsterCreator::set_params -- unknown key '{}'", key));
}

TileRef MonsterCreator::get_tile(std::string_view key)
{
	if (auto id = key_to_standard_id(key))
	{
		return get_tile(*id);
	}

	if (auto id = key_to_class_id(key))
	{
		return get_tile(*id);
	}

	if (s_custom.contains(std::string{ key }))
	{
		return s_custom.at(std::string{ key }).symbol;
	}

	return TileRef{};
}

void MonsterCreator::set_tile(std::string_view key, TileRef tile)
{
	if (auto id = key_to_standard_id(key))
	{
		set_tile(*id, tile);
		return;
	}

	if (auto id = key_to_class_id(key))
	{
		set_tile(*id, tile);
		return;
	}

	if (s_custom.contains(std::string{ key }))
	{
		s_custom[std::string{ key }].symbol = tile;
		return;
	}

	throw std::out_of_range(std::format("MonsterCreator::set_tile -- unknown key '{}'", key));
}

std::string MonsterCreator::add_custom(MonsterParams p)
{
	std::string base = normalize_key(p.name.empty() ? "new_monster" : p.name);
	std::string key = unique_key(base);
	s_custom[key] = std::move(p);

	return key;
}

void MonsterCreator::remove_custom(std::string_view key)
{
	if (key_to_standard_id(key) || key_to_class_id(key))
	{
		throw std::invalid_argument(
			std::format("MonsterCreator::remove_custom -- '{}' is a built-in monster and cannot be removed", key));
	}

	if (!s_custom.erase(std::string{ key }))
	{
		throw std::out_of_range(
			std::format("MonsterCreator::remove_custom -- unknown key '{}'", key));
	}
}

bool MonsterCreator::is_builtin(std::string_view key)
{
	return key_to_standard_id(key).has_value();
}

bool MonsterCreator::is_class_key(std::string_view key)
{
	return key_to_class_id(key).has_value();
}

// end of file: MonsterCreator.cpp
