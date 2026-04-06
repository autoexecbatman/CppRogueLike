#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "../Actor/Actor.h"
#include "../Actor/EquipmentSlot.h"
#include "../Actor/Pickable.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Core/Paths.h"
#include "../Items/MagicalItemEffects.h"
#include "../Map/Map.h"
#include "../Menu/MenuSpellCast.h"
#include "../Utils/Vector2D.h"
#include "AnimationSystem.h"
#include "BuffSystem.h"
#include "BuffType.h"
#include "CreatureManager.h"
#include "MessageSystem.h"
#include "SpellAnimations.h"
#include "SpellSystem.h"
#include "TargetingMenu.h"
#include "TargetingSystem.h"
#include "TileConfig.h"

// Helper to convert PlayerClassState to CasterClass
static CasterClass to_caster_class(Player::PlayerClassState state)
{
	switch (state)
	{

	case Player::PlayerClassState::CLERIC:
	{
		return CasterClass::CLERIC;
	}

	case Player::PlayerClassState::WIZARD:
	{
		return CasterClass::WIZARD;
	}

	default:
	{
		return CasterClass::NONE;
	}

	}
}

// ---------------------------------------------------------------------------
// Module-level mutable spell table. Loaded from JSON; falls back to defaults
// if load() has not been called (e.g. in unit tests).
// ---------------------------------------------------------------------------

namespace
{

// Implementation-only identifier used to key s_spells and SPELL_KEYS.
// Not part of the public API -- the game addresses spells by string key.
enum class SpellId
{
	CURE_LIGHT_WOUNDS,
	BLESS,
	SANCTUARY,
	HOLD_PERSON,
	SILENCE,
	MAGIC_MISSILE,
	SHIELD,
	SLEEP,
	INVISIBILITY,
	WEB,
	FIREBALL,
	TELEPORT,
	NONE
};

std::map<std::string, SpellDefinition> s_custom_spells;

std::map<SpellId, SpellDefinition> s_spells = {
	{ SpellId::CURE_LIGHT_WOUNDS, { "Cure Light Wounds", 1, SpellClass::CLERIC, "Heals 1d8 HP", SpellEffectType::CURE_LIGHT_WOUNDS } },
	{ SpellId::BLESS, { "Bless", 1, SpellClass::CLERIC, "+1 to hit for 6 turns", SpellEffectType::BLESS } },
	{ SpellId::SANCTUARY, { "Sanctuary", 1, SpellClass::CLERIC, "Enemies ignore you for 3 turns", SpellEffectType::SANCTUARY } },
	{ SpellId::HOLD_PERSON, { "Hold Person", 2, SpellClass::CLERIC, "Paralyze target for 4 turns", SpellEffectType::HOLD_PERSON } },
	{ SpellId::SILENCE, { "Silence", 2, SpellClass::CLERIC, "Prevent target from casting", SpellEffectType::SILENCE } },
	{ SpellId::MAGIC_MISSILE, { "Magic Missile", 1, SpellClass::WIZARD, "1d4+1 force damage, auto-hit", SpellEffectType::MAGIC_MISSILE } },
	{ SpellId::SHIELD, { "Shield", 1, SpellClass::WIZARD, "+4 AC for 5 turns", SpellEffectType::SHIELD } },
	{ SpellId::SLEEP, { "Sleep", 1, SpellClass::WIZARD, "Put weak enemies to sleep", SpellEffectType::SLEEP } },
	{ SpellId::INVISIBILITY, { "Invisibility", 2, SpellClass::WIZARD, "Become invisible for 20 turns", SpellEffectType::INVISIBILITY } },
	{ SpellId::WEB, { "Web", 2, SpellClass::WIZARD, "Create webs to trap enemies", SpellEffectType::WEB } },
	{ SpellId::FIREBALL, { "Fireball", 3, SpellClass::WIZARD, "1d6/level fire damage in 20-ft radius, save vs. spells for half", SpellEffectType::FIREBALL } },
	{ SpellId::TELEPORT, { "Teleport", 3, SpellClass::WIZARD, "Teleport to random location", SpellEffectType::TELEPORT } },
	{ SpellId::NONE, { "None", 0, SpellClass::BOTH, "", SpellEffectType::NONE } },
};

// JSON key <-> SpellId mapping
struct SpellEntry
{
	SpellId id{};
	std::string_view key{};
};

constexpr SpellEntry SPELL_KEYS[] = {
	{ SpellId::CURE_LIGHT_WOUNDS, "cure_light_wounds" },
	{ SpellId::BLESS, "bless" },
	{ SpellId::SANCTUARY, "sanctuary" },
	{ SpellId::HOLD_PERSON, "hold_person" },
	{ SpellId::SILENCE, "silence" },
	{ SpellId::MAGIC_MISSILE, "magic_missile" },
	{ SpellId::SHIELD, "shield" },
	{ SpellId::SLEEP, "sleep" },
	{ SpellId::INVISIBILITY, "invisibility" },
	{ SpellId::WEB, "web" },
	{ SpellId::FIREBALL, "fireball" },
	{ SpellId::TELEPORT, "teleport" },
};

SpellClass parse_class(std::string_view s)
{
	if (s == "cleric")
	{
		return SpellClass::CLERIC;
	}
	else if (s == "wizard")
	{
		return SpellClass::WIZARD;
	}
	else
	{
		return SpellClass::BOTH;
	}
}

std::string encode_class(SpellClass c)
{
	if (c == SpellClass::CLERIC)
	{
		return "cleric";
	}
	else if (c == SpellClass::WIZARD)
	{
		return "wizard";
	}
	else
	{
		return "both";
	}
}

SpellEffectType parse_effect_type(std::string_view s)
{
	if (s == "cure_light_wounds")
	{
		return SpellEffectType::CURE_LIGHT_WOUNDS;
	}
	else if (s == "bless")
	{
		return SpellEffectType::BLESS;
	}
	else if (s == "sanctuary")
	{
		return SpellEffectType::SANCTUARY;
	}
	else if (s == "hold_person")
	{
		return SpellEffectType::HOLD_PERSON;
	}
	else if (s == "silence")
	{
		return SpellEffectType::SILENCE;
	}
	else if (s == "magic_missile")
	{
		return SpellEffectType::MAGIC_MISSILE;
	}
	else if (s == "shield")
	{
		return SpellEffectType::SHIELD;
	}
	else if (s == "sleep")
	{
		return SpellEffectType::SLEEP;
	}
	else if (s == "invisibility")
	{
		return SpellEffectType::INVISIBILITY;
	}
	else if (s == "web")
	{
		return SpellEffectType::WEB;
	}
	else if (s == "fireball")
	{
		return SpellEffectType::FIREBALL;
	}
	else if (s == "teleport")
	{
		return SpellEffectType::TELEPORT;
	}
	else
	{
		return SpellEffectType::NONE;
	}
}

std::string encode_effect_type(SpellEffectType e)
{
	switch (e)
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

	default:
	{
		return "none";
	}

	}
}

} // namespace

// ---------------------------------------------------------------------------

void SpellSystem::load(std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::ifstream f(resolved);
	if (!f.is_open())
	{
		throw std::runtime_error(
			std::format("SpellSystem::load -- cannot open '{}'", resolved.string()));
	}

	nlohmann::json root = nlohmann::json::parse(f);

	s_custom_spells.clear();

	// Load known builtin keys
	for (const auto& entry : SPELL_KEYS)
	{
		const std::string key{ entry.key };
		if (!root.contains(key))
		{
			continue;
		}
		const auto& j = root.at(key);
		SpellDefinition& def = s_spells.at(entry.id);
		def.name = j.at("name").get<std::string>();
		def.level = j.at("level").get<int>();
		def.spellClass = parse_class(j.at("class").get<std::string>());
		def.description = j.at("description").get<std::string>();
	}

	// Load custom spells: any key not in SPELL_KEYS
	for (const auto& [key, j] : root.items())
	{
		bool is_builtin = false;
		for (const auto& entry : SPELL_KEYS)
		{
			if (entry.key == key)
			{
				is_builtin = true;
				break;
			}
		}
		if (is_builtin)
		{
			continue;
		}

		SpellDefinition def;
		def.name = j.at("name").get<std::string>();
		def.level = j.at("level").get<int>();
		def.spellClass = parse_class(j.at("class").get<std::string>());
		def.description = j.at("description").get<std::string>();
		if (j.contains("effect"))
		{
			def.effect_type = parse_effect_type(j.at("effect").get<std::string>());
		}
		s_custom_spells[key] = std::move(def);
	}
}

void SpellSystem::save(std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::filesystem::create_directories(resolved.parent_path());

	nlohmann::json root = nlohmann::json::object();

	for (const auto& entry : SPELL_KEYS)
	{
		const SpellDefinition& def = s_spells.at(entry.id);
		root[std::string{ entry.key }] = nlohmann::json{
			{ "name", def.name },
			{ "level", def.level },
			{ "class", encode_class(def.spellClass) },
			{ "description", def.description }
		};
	}

	for (const auto& [key, def] : s_custom_spells)
	{
		root[key] = nlohmann::json{
			{ "name", def.name },
			{ "level", def.level },
			{ "class", encode_class(def.spellClass) },
			{ "description", def.description },
			{ "effect", encode_effect_type(def.effect_type) }
		};
	}

	std::ofstream f(resolved);
	if (!f.is_open())
	{
		throw std::runtime_error(
			std::format("SpellSystem::save -- cannot open '{}' for writing", resolved.string()));
	}
	f << root.dump(4);
	if (f.fail())
	{
		throw std::runtime_error(
			std::format("SpellSystem::save -- write failed for '{}'", resolved.string()));
	}
}

// ---------------------------------------------------------------------------
// String-keyed API (editor)
// ---------------------------------------------------------------------------

std::vector<std::string> SpellSystem::get_all_keys()
{
	std::vector<std::string> keys;
	keys.reserve(std::size(SPELL_KEYS) + s_custom_spells.size());
	for (const auto& entry : SPELL_KEYS)
	{
		keys.push_back(std::string{ entry.key });
	}
	for (const auto& [key, _] : s_custom_spells)
	{
		keys.push_back(key);
	}
	return keys;
}

const SpellDefinition& SpellSystem::get_by_key(std::string_view key)
{
	for (const auto& entry : SPELL_KEYS)
	{
		if (entry.key == key)
		{
			return s_spells.at(entry.id);
		}
	}

	auto it = s_custom_spells.find(std::string{ key });
	if (it != s_custom_spells.end())
	{
		return it->second;
	}

	throw std::out_of_range(std::format("SpellSystem::get_by_key -- unknown key '{}'", key));
}

void SpellSystem::set_by_key(std::string_view key, const SpellDefinition& def)
{
	for (const auto& entry : SPELL_KEYS)
	{
		if (entry.key == key)
		{
			s_spells[entry.id] = def;
			return;
		}
	}

	auto it = s_custom_spells.find(std::string{ key });
	if (it != s_custom_spells.end())
	{
		it->second = def;
		return;
	}

	throw std::out_of_range(std::format("SpellSystem::set_by_key -- unknown key '{}'", key));
}

std::string SpellSystem::add_custom(SpellDefinition def)
{
	auto normalize = [](std::string_view name) -> std::string
	{
		std::string key;
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
	};

	auto has_key = [](const std::string& k) -> bool
	{
		for (const auto& entry : SPELL_KEYS)
		{
			if (entry.key == k)
			{
				return true;
			}
		}

		return s_custom_spells.contains(k);
	};

	std::string base = normalize(def.name.empty() ? "new_spell" : def.name);
	std::string key = base;
	if (has_key(key))
	{
		for (int n = 2;; ++n)
		{
			key = std::format("{}_{}", base, n);
			if (!has_key(key))
			{
				break;
			}
		}
	}
	s_custom_spells[key] = std::move(def);

	return key;
}

void SpellSystem::remove_custom(std::string_view key)
{
	for (const auto& entry : SPELL_KEYS)
	{
		if (entry.key == key)
		{
			throw std::invalid_argument(
				std::format("SpellSystem::remove_custom -- '{}' is a built-in spell and cannot be removed", key));
		}
	}
	if (!s_custom_spells.erase(std::string{ key }))
	{
		throw std::out_of_range(
			std::format("SpellSystem::remove_custom -- unknown key '{}'", key));
	}
}

bool SpellSystem::is_builtin_key(std::string_view key)
{
	for (const auto& entry : SPELL_KEYS)
	{
		if (entry.key == key)
		{
			return true;
		}
	}
	return false;
}

std::vector<int> SpellSystem::get_spell_slots(CasterClass classState, int level)
{
	// AD&D 2e spell progression tables
	// Returns slots per spell level [level1, level2, level3, ...]

	if (classState == CasterClass::CLERIC)
	{
		// Cleric spell progression
		static const std::vector<std::vector<int>> clericSlots = {
			{ 1 }, // Level 1
			{ 2 }, // Level 2
			{ 2, 1 }, // Level 3
			{ 3, 2 }, // Level 4
			{ 3, 3, 1 }, // Level 5
			{ 3, 3, 2 }, // Level 6
			{ 3, 3, 2, 1 }, // Level 7
			{ 3, 3, 3, 2 }, // Level 8
			{ 4, 4, 3, 2, 1 }, // Level 9
			{ 4, 4, 3, 3, 2 }, // Level 10
		};
		int idx = std::min(level, 10) - 1;
		return idx >= 0 ? clericSlots[idx] : std::vector<int>{};
	}
	else if (classState == CasterClass::WIZARD)
	{
		// Wizard spell progression
		static const std::vector<std::vector<int>> wizardSlots = {
			{ 1 }, // Level 1
			{ 2 }, // Level 2
			{ 2, 1 }, // Level 3
			{ 3, 2 }, // Level 4
			{ 4, 2, 1 }, // Level 5
			{ 4, 2, 2 }, // Level 6
			{ 4, 3, 2, 1 }, // Level 7
			{ 4, 3, 3, 2 }, // Level 8
			{ 4, 3, 3, 2, 1 }, // Level 9
			{ 4, 4, 3, 2, 2 }, // Level 10
		};
		int idx = std::min(level, 10) - 1;
		return idx >= 0 ? wizardSlots[idx] : std::vector<int>{};
	}

	return {};
}

std::vector<std::string> SpellSystem::get_available_spells(CasterClass classState, int maxSpellLevel)
{
	std::vector<std::string> available;
	SpellClass targetClass = (classState == CasterClass::CLERIC) ? SpellClass::CLERIC : SpellClass::WIZARD;

	for (const auto& entry : SPELL_KEYS)
	{
		const SpellDefinition& def = s_spells.at(entry.id);
		if (entry.id == SpellId::NONE)
		{
			continue;
		}
		if ((def.spellClass == targetClass || def.spellClass == SpellClass::BOTH) && def.level <= maxSpellLevel)
		{
			available.push_back(std::string{ entry.key });
		}
	}

	for (const auto& [key, def] : s_custom_spells)
	{
		if ((def.spellClass == targetClass || def.spellClass == SpellClass::BOTH) && def.level <= maxSpellLevel)
		{
			available.push_back(key);
		}
	}

	return available;
}

void SpellSystem::dispatch_effect(
	SpellEffectType effect,
	Creature& caster,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	// Targeted spells: async via TargetingMenu — onSuccess is forwarded into the callback
	switch (effect)
	{

	case SpellEffectType::SILENCE:
	{
		cast_silence(caster, std::move(onSuccess), ctx);
		return;
	}

	case SpellEffectType::WEB:
	{
		cast_web(caster, std::move(onSuccess), ctx);
		return;
	}

	case SpellEffectType::FIREBALL:
	{
		cast_fireball(caster, std::move(onSuccess), ctx);
		return;
	}

	default:
		break;

	}

	// Instant spells: cast synchronously, call onSuccess if successful
	bool result = false;
	switch (effect)
	{

	case SpellEffectType::CURE_LIGHT_WOUNDS:
	{
		result = cast_cure_light_wounds(caster, ctx);
		break;
	}

	case SpellEffectType::BLESS:
	{
		result = cast_bless(caster, ctx);
		break;
	}

	case SpellEffectType::SANCTUARY:
	{
		result = cast_sanctuary(caster, ctx);
		break;
	}

	case SpellEffectType::HOLD_PERSON:
	{
		result = cast_hold_person(caster, ctx);
		break;
	}

	case SpellEffectType::MAGIC_MISSILE:
	{
		result = cast_magic_missile(caster, ctx);
		break;
	}

	case SpellEffectType::SHIELD:
	{
		result = cast_shield(caster, ctx);
		break;
	}

	case SpellEffectType::SLEEP:
	{
		result = cast_sleep(caster, ctx);
		break;
	}

	case SpellEffectType::INVISIBILITY:
	{
		result = cast_invisibility(caster, ctx);
		break;
	}

	case SpellEffectType::TELEPORT:
	{
		result = cast_teleport(caster, ctx);
		break;
	}

	default:
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Spell not implemented yet.", true);
		break;
	}

	}

	if (result)
	{
		onSuccess(ctx);
	}
}

void SpellSystem::cast_spell_by_key(
	std::string_view key,
	Creature& caster,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	if (caster.has_state(ActorState::IS_SILENCED))
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You are silenced and cannot cast spells!", true);
		return;
	}
	const SpellDefinition& def = get_by_key(key);
	dispatch_effect(def.effect_type, caster, std::move(onSuccess), ctx);
}

static void animate_heal(const Vector2D& pos, GameContext& ctx)
{
	if (!ctx.animSystem)
	{
		return;
	}

	ctx.animSystem->spawn_effect(
		pos.x,
		pos.y,
		ctx.tileConfig->get("TILE_EFFECT_HEAL"),
		60,
		220,
		60,
		0.5f);
}

bool SpellSystem::cast_cure_light_wounds(Creature& caster, GameContext& ctx)
{
	if (!caster.destructible)
	{
		return false;
	}

	animate_heal(caster.position, ctx);

	int healing = ctx.dice->roll(1, 8);
	int oldHp = caster.destructible->get_hp();
	int maxHp = caster.destructible->get_max_hp();
	int newHp = std::min(oldHp + healing, maxHp);
	int actualHealing = newHp - oldHp;

	caster.destructible->set_hp(newHp);

	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Cure Light Wounds! ");
	ctx.messageSystem->append_message_part(GREEN_BLACK_PAIR, std::format("+{} HP", actualHealing));
	ctx.messageSystem->finalize_message();

	return true;
}

bool SpellSystem::cast_bless(Creature& caster, GameContext& ctx)
{
	ctx.buffSystem->add_buff(caster, BuffType::BLESS, 0, 6, false); // Spell: ADD effect
	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Bless! ");
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "+1 to hit for 6 turns.");
	ctx.messageSystem->finalize_message();
	return true;
}

bool SpellSystem::cast_sanctuary(Creature& caster, GameContext& ctx)
{
	// AD&D 2e: Duration 3 rounds + 1 round/level. Cancelled by attacking.
	auto* player = dynamic_cast<Player*>(&caster);
	int casterLevel = player ? player->get_creature_level() : 1;
	int duration = 3 + casterLevel;

	ctx.buffSystem->add_buff(caster, BuffType::SANCTUARY, 0, duration, false);

	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Sanctuary! ");
	ctx.messageSystem->append_message_part(
		WHITE_BLACK_PAIR,
		std::format("Divine protection shields you for {} turns.", duration));
	ctx.messageSystem->finalize_message();
	return true;
}

void SpellSystem::cast_silence(
	Creature& caster,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	// AD&D 2e: Duration 2 rounds/level. Targeted single creature. Prevents spellcasting.
	auto* player = dynamic_cast<Player*>(&caster);
	int casterLevel = player ? player->get_creature_level() : 1;
	int duration = 2 * casterLevel;
	int range = 5 + casterLevel;

	auto onTarget = [&caster, duration, onSuccess = std::move(onSuccess)](
		bool confirmed,
		Vector2D targetPos,
		GameContext& innerCtx) mutable
	{
		if (!confirmed)
		{
			innerCtx.messageSystem->message(WHITE_BLACK_PAIR, "Silence cancelled.", true);
			return;
		}

		Creature* target = nullptr;
		for (const auto& creature : *innerCtx.creatures)
		{
			if (creature && creature->position == targetPos && !creature->destructible->is_dead())
			{
				target = creature.get();
				break;
			}
		}

		if (!target)
		{
			innerCtx.messageSystem->message(WHITE_BLACK_PAIR, "No creature at that location.", true);
			return;
		}

		innerCtx.buffSystem->add_buff(*target, BuffType::SILENCE, 0, duration, false);
		SpellAnimations::animate_creature_hit(target->position, innerCtx);

		innerCtx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Silence! ");
		innerCtx.messageSystem->append_message_part(
			WHITE_BLACK_PAIR,
			std::format("{} is struck mute for {} turns.", target->get_name(), duration));
		innerCtx.messageSystem->finalize_message();

		onSuccess(innerCtx);
	};

	ctx.menus->push_back(std::make_unique<TargetingMenu>(range, 0, std::move(onTarget), ctx));
}

void SpellSystem::cast_web(
	Creature& caster,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	// AD&D 2e: Save vs. Paralyzation (d20 >= 10) or be entangled. Duration 2 rounds/level.
	auto* player = dynamic_cast<Player*>(&caster);
	int casterLevel = player ? player->get_creature_level() : 1;
	int range = 5 * casterLevel;
	int radius = 2;
	int duration = 2 * casterLevel;

	auto onTarget = [radius, duration, onSuccess = std::move(onSuccess)](
		bool confirmed,
		Vector2D center,
		GameContext& innerCtx) mutable
	{
		if (!confirmed)
		{
			innerCtx.messageSystem->message(WHITE_BLACK_PAIR, "Web cancelled.", true);
			return;
		}

		int affected = 0;
		for (const auto& creature : *innerCtx.creatures)
		{
			if (!creature || creature->destructible->is_dead())
			{
				continue;
			}
			if (creature->get_tile_distance(center) > radius)
			{
				continue;
			}

			// AD&D 2e: Save vs. Paralyzation (d20 >= 10) avoids entanglement
			int save = innerCtx.dice->roll(1, 20);
			if (save < 10)
			{
				innerCtx.buffSystem->add_buff(*creature, BuffType::WEBBED, 0, duration, false);
				SpellAnimations::animate_creature_hit(creature->position, innerCtx);
				++affected;
			}
		}

		if (affected > 0)
		{
			innerCtx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Web! ");
			innerCtx.messageSystem->append_message_part(
				WHITE_BLACK_PAIR,
				std::format(
					"{} creature{} entangled for {} turns.",
					affected,
					affected == 1 ? " is" : "s are",
					duration));
			innerCtx.messageSystem->finalize_message();
		}
		else
		{
			innerCtx.messageSystem->message(WHITE_BLACK_PAIR, "The webs spread but catch nothing.", true);
		}

		onSuccess(innerCtx);
	};

	ctx.menus->push_back(std::make_unique<TargetingMenu>(range, radius, std::move(onTarget), ctx));
}

void SpellSystem::cast_fireball(
	Creature& caster,
	std::function<void(GameContext&)> onSuccess,
	GameContext& ctx)
{
	auto* player = dynamic_cast<Player*>(&caster);
	int casterLevel = player ? player->get_creature_level() : 1;

	// AD&D 2e: range 10" + 1"/level (tiles), AoE 20-ft radius (2 tiles)
	int range = 10 + casterLevel;
	int radius = 2;

	auto onTarget = [casterLevel, radius, onSuccess = std::move(onSuccess)](
		bool confirmed,
		Vector2D center,
		GameContext& innerCtx) mutable
	{
		if (!confirmed)
		{
			innerCtx.messageSystem->message(WHITE_BLACK_PAIR, "Fireball cancelled.", true);
			return;
		}

		SpellAnimations::animate_explosion(center, radius, innerCtx);

		// AD&D 2e: 1d6 per caster level, max 10d6
		int diceCnt = std::min(casterLevel, 10);
		int totalDamage = 0;
		for (int i = 0; i < diceCnt; ++i)
		{
			totalDamage += innerCtx.dice->roll(1, 6);
		}

		int affected = 0;
		for (const auto& creature : *innerCtx.creatures)
		{
			if (!creature || creature->destructible->is_dead())
			{
				continue;
			}
			if (creature->get_tile_distance(center) > static_cast<double>(radius))
			{
				continue;
			}

			// AD&D 2e: Save vs. Spells (d20 >= 15) for half damage
			int save = innerCtx.dice->roll(1, 20);
			int dealt = (save >= 15) ? totalDamage / 2 : totalDamage;
			creature->destructible->take_damage(*creature, dealt, innerCtx);
			++affected;
		}

		innerCtx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, "Fireball! ");
		innerCtx.messageSystem->append_message_part(RED_BLACK_PAIR, std::format("{}d6 = {} damage", diceCnt, totalDamage));
		if (affected > 0)
		{
			innerCtx.messageSystem->append_message_part(WHITE_BLACK_PAIR, std::format(" ({} struck)", affected));
		}
		innerCtx.messageSystem->finalize_message();

		innerCtx.creatureManager->cleanup_dead_creatures(*innerCtx.creatures);

		onSuccess(innerCtx);
	};

	ctx.menus->push_back(std::make_unique<TargetingMenu>(range, radius, std::move(onTarget), ctx));
}

static void animate_magic_missile(const Vector2D& to, GameContext& ctx)
{
	SpellAnimations::animate_creature_hit(to, ctx);
}

static int calculate_num_missiles(int casterLevel)
{
	// AD&D 2e: 1 missile at level 1, +1 every 2 levels, max 5
	return std::min(5, 1 + (casterLevel - 1) / 2);
}

bool SpellSystem::cast_magic_missile(Creature& caster, GameContext& ctx)
{
	// Get caster level
	auto* player = dynamic_cast<Player*>(&caster);
	int casterLevel = player ? player->get_creature_level() : 1;
	int numMissiles = calculate_num_missiles(casterLevel);

	// Find all valid targets in FOV
	std::vector<Creature*> targets;
	for (const auto& creature : *ctx.creatures)
	{
		if (creature && creature->destructible && !creature->destructible->is_dead())
		{
			if (ctx.map->is_in_fov(creature->position))
			{
				targets.push_back(creature.get());
			}
		}
	}

	if (targets.empty())
	{
		ctx.messageSystem->message(RED_BLACK_PAIR, "No valid target in sight!", true);
		return false;
	}

	// Sort by distance (nearest first)
	std::sort(targets.begin(), targets.end(), [&caster](Creature* a, Creature* b)
		{ return caster.get_tile_distance(a->position) < caster.get_tile_distance(b->position); });

	int totalDamage = 0;
	std::unordered_map<Creature*, int> damagePerTarget;

	// Fire missiles - distribute among targets, prioritizing nearest
	for (int i = 0; i < numMissiles; ++i)
	{
		// Target nearest living enemy
		Creature* target = nullptr;
		for (Creature* t : targets)
		{
			if (t->destructible && !t->destructible->is_dead())
			{
				target = t;
				break;
			}
		}

		if (!target)
		{
			break;
		}

		animate_magic_missile(target->position, ctx);

		int damage = ctx.dice->roll(1, 4) + 1;
		totalDamage += damage;
		damagePerTarget[target] += damage;

		target->destructible->take_damage(*target, damage, ctx);
	}

	// Message
	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, std::format("Magic Missile ({})! ", numMissiles));
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "Total ");
	ctx.messageSystem->append_message_part(RED_BLACK_PAIR, std::format("{} damage!", totalDamage));
	ctx.messageSystem->finalize_message();

	ctx.creatureManager->cleanup_dead_creatures(*ctx.creatures);

	return true;
}

bool SpellSystem::cast_shield(Creature& caster, GameContext& ctx)
{
	ctx.buffSystem->add_buff(caster, BuffType::SHIELD, 4, 5, false); // Spell: ADD +4 AC
	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Shield! ");
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "+4 AC for 5 turns.");
	ctx.messageSystem->finalize_message();
	return true;
}

bool SpellSystem::cast_sleep(Creature& caster, GameContext& ctx)
{
	// AD&D 2e: 2d8 HD of creatures affected, lowest HD first
	int hdBudget = ctx.dice->roll(2, 8);
	int affected = 0;

	for (const auto& creature : *ctx.creatures)
	{
		if (hdBudget <= 0)
		{
			break;
		}
		if (!creature || !creature->destructible || creature->destructible->is_dead())
		{
			continue;
		}
		if (!ctx.map->is_in_fov(creature->position))
		{
			continue;
		}

		// Proxy HD from max HP (4 HP per HD)
		int hd = std::max(1, creature->destructible->get_max_hp() / 4);
		if (hd <= hdBudget)
		{
			ctx.buffSystem->add_buff(*creature, BuffType::SLEEP, 0, 5, false);
			hdBudget -= hd;
			++affected;
		}
	}

	if (affected > 0)
	{
		ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Sleep! ");
		ctx.messageSystem->append_message_part(
			WHITE_BLACK_PAIR, std::format("{} creatures fall asleep.", affected));
		ctx.messageSystem->finalize_message();
	}
	else
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Sleep spell has no effect.", true);
	}

	return true;
}

bool SpellSystem::cast_hold_person(Creature& caster, GameContext& ctx)
{
	// AD&D 2e: paralyzes up to 1d4 humanoids in FOV; save vs. spells (d20 >= 15) negates
	// Duration: 2 rounds per caster level
	auto* player = dynamic_cast<Player*>(&caster);
	int casterLevel = player ? player->get_creature_level() : 1;
	int duration = 2 * casterLevel;
	int maxTargets = ctx.dice->roll(1, 4);
	int affected = 0;

	for (const auto& creature : *ctx.creatures)
	{
		if (affected >= maxTargets)
		{
			break;
		}
		if (!creature || !creature->destructible || creature->destructible->is_dead())
		{
			continue;
		}
		if (!ctx.map->is_in_fov(creature->position))
		{
			continue;
		}

		int save = ctx.dice->roll(1, 20);
		if (save < 15)
		{
			ctx.buffSystem->add_buff(*creature, BuffType::HOLD_PERSON, 0, duration, false);
			++affected;
		}
	}

	if (affected > 0)
	{
		ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Hold Person! ");
		ctx.messageSystem->append_message_part(
			WHITE_BLACK_PAIR, std::format("{} creatures paralyzed for {} turns.", affected, duration));
		ctx.messageSystem->finalize_message();
	}
	else
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Hold Person has no effect.", true);
	}

	return true;
}

bool SpellSystem::cast_invisibility(Creature& caster, GameContext& ctx)
{
	ctx.buffSystem->add_buff(caster, BuffType::INVISIBILITY, 0, 20, false); // Spell: ADD effect
	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Invisibility! ");
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You fade from view for 20 turns.");
	ctx.messageSystem->finalize_message();
	return true;
}

bool SpellSystem::cast_teleport(Creature& caster, GameContext& ctx)
{
	// Try to find a valid teleport location (up to 50 attempts)
	for (int attempts = 0; attempts < 50; attempts++)
	{
		int x = ctx.dice->roll(2, ctx.map->get_width() - 2);
		int y = ctx.dice->roll(2, ctx.map->get_height() - 2);
		Vector2D teleportPos{ y, x };

		// Check if the tile is a floor and walkable
		if (ctx.map->get_tile_type(teleportPos) == TileType::FLOOR && ctx.map->can_walk(teleportPos, ctx))
		{
			// Check if any creature is at this position
			bool occupied = false;
			for (const auto& creature : *ctx.creatures)
			{
				if (creature && creature->position == teleportPos)
				{
					occupied = true;
					break;
				}
			}

			if (!occupied)
			{
				// Teleport successful
				caster.position = teleportPos;
				ctx.map->compute_fov(ctx);

				ctx.messageSystem->append_message_part(MAGENTA_BLACK_PAIR, "Teleport! ");
				ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You feel disoriented as the world shifts around you!");
				ctx.messageSystem->finalize_message();
				return true;
			}
		}
	}

	// Failed to find a valid location
	ctx.messageSystem->message(RED_BLACK_PAIR, "The teleportation magic fizzles out - no safe location found!", true);
	return false;
}

void SpellSystem::show_memorization_menu(Player& player, GameContext& ctx)
{
	CasterClass casterClass = to_caster_class(player.playerClassState);
	auto slots = get_spell_slots(casterClass, player.get_creature_level());
	if (slots.empty())
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You cannot cast spells.", true);
		return;
	}

	int maxSpellLevel = static_cast<int>(slots.size());
	auto available = get_available_spells(casterClass, maxSpellLevel);

	// Clear current memorized spells
	player.memorizedSpells.clear();

	// Auto-memorize spells to fill slots (simplified)
	for (int level = 1; level <= maxSpellLevel; ++level)
	{
		int slotsAtLevel = slots[level - 1];
		for (const std::string& key : available)
		{
			const SpellDefinition& def = get_by_key(key);
			if (def.level == level && slotsAtLevel > 0)
			{
				player.memorizedSpells.push_back(key);
				--slotsAtLevel;
			}
		}
	}

	ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, "Spells memorized: ");
	for (size_t i = 0; i < player.memorizedSpells.size(); ++i)
	{
		if (i > 0)
		{
			ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ", ");
		}
		ctx.messageSystem->append_message_part(GREEN_BLACK_PAIR, get_by_key(player.memorizedSpells[i]).name);
	}
	ctx.messageSystem->finalize_message();
}

void SpellSystem::show_casting_menu(Player& player, GameContext& ctx)
{
	ctx.menus->push_back(std::make_unique<MenuSpellCast>(player));
}

std::vector<SpellSystem::ItemGrantedSpell> SpellSystem::get_item_granted_spells(const Player& player)
{
	std::vector<ItemGrantedSpell> itemSpells;

	// Check for Ring of Invisibility
	for (auto slot : { EquipmentSlot::RIGHT_RING, EquipmentSlot::LEFT_RING })
	{
		if (Item* ring = player.get_equipped_item(slot))
		{
			if (const auto* magicRing = ring->behavior ? std::get_if<MagicalRing>(&*ring->behavior) : nullptr)
			{
				if (magicRing->effect == MagicalEffect::INVISIBILITY)
				{
					itemSpells.push_back({ "invisibility", "Ring" });
					break; // Only add once even if wearing two
				}
			}
		}
	}

	// Check for Helm of Teleportation
	if (Item* helm = player.get_equipped_item(EquipmentSlot::HEAD))
	{
		if (const auto* magicHelm = helm->behavior ? std::get_if<MagicalHelm>(&*helm->behavior) : nullptr)
		{
			if (magicHelm->effect == MagicalEffect::TELEPORTATION)
			{
				itemSpells.push_back({ "teleport", "Helm" });
			}
		}
	}

	return itemSpells;
}
