#pragma once

#include "Colors.h"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Ai.h"
#include "Alignment.h"
#include "Attitude.h"
#include "CorpseEdibility.h"
#include "CreatureSize.h"
#include "DamageInfo.h"
#include "DiceExpr.h"
#include "EquipmentSlot.h"
#include "IntelligenceRating.h"
#include "MonsterHitDice.h"
#include "MonsterStrength.h"
#include "SpellRegistry.h"
#include "Spellcasting.h"
#include "TileRef.h"

// file: MonsterRegistry.h
//
// Every monster the game knows, by key: the standard monsters, which the code names by
// MonsterId, and the custom monsters the monster editor adds, all with their full
// parameters from monsters.json. Game
// owns the registry the game plays with and hands it out as ctx.monsterRegistry;
// MonsterCreator builds creatures from what it holds and MonsterFactory draws spawns
// from it. Each registry is its own value, so a test builds one and an edit to it
// reaches nothing else.
//
// Usage:
//
//   MonsterRegistry monsters;
//   monsters.load("data/content/monsters.json"); // throws if missing or a standard monster has no record
//   monsters.get_params("goblin").thaco; // -> 20
//   monsters.add_custom(MonsterParams{ .name = "Registry Beast" }); // -> "registry_beast"
//   monsters.remove_custom("goblin"); // throws std::invalid_argument, a builtin
//   monsters.get_params("no_such_monster"); // throws std::out_of_range
//   monsters.save("data/content/monsters.json");

// The monsters with a compiled-in identity, which code refers to by name: the shopkeeper's
// spawn gate and the mimic's revealed tile among them.
enum class MonsterId
{
	GOBLIN,
	ORC,
	TROLL,
	DRAGON,
	ARCHER,
	MAGE,
	WOLF,
	FIRE_WOLF,
	ICE_WOLF,
	BAT,
	KOBOLD,
	MIMIC,
	SHOPKEEPER,
	SPIDER_SMALL,
	SPIDER_GIANT,
	SPIDER_WEAVER,
};

// Everything a monster built from data is made of. Dice expressions roll num dice of
// sides sides and add bonus; num 0 means skip.
struct MonsterParams
{
	// Identity
	TileRef symbol{};
	// The name messages and menus show; a standard monster's is read from its corpse.
	std::string name;
	// The colour pair it is drawn in.
	ColorPairId color{ ColorPairId::WHITE_BLACK };
	// What its corpse is called.
	std::string corpseName;

	// Combat
	// Its hit dice and the experience each count is worth.
	MonsterHitDice hitDice{};
	// The d20 roll it needs to hit armour class 0.
	int thaco{ 20 };
	// Its armour class; lower is harder to hit.
	int ac{ 10 };
	// Its damage reduction.
	int dr{ 0 };
	// The morale score its flight is rolled against.
	int morale{ 10 };
	// Whether it is undead, and so can be turned.
	bool undead{ false };
	// Its alignment, law against chaos.
	Ethics ethics{ Ethics::NEUTRAL };
	// Its alignment, good against evil.
	Morality morality{ Morality::NEUTRAL };
	// What its corpse weighs.
	int corpseWeight{ 50 };
	// How much eating its corpse feeds, whether it can be eaten at all, and the line printed
	// when it is eaten or refused.
	int corpseNutrition{ 0 };
	CorpseEdibility corpseEdibility{ CorpseEdibility::EDIBLE };
	std::string corpseTaste{};
	// The size its Monstrous Manual entry prints, which the DMG reads its ability scores by.
	CreatureSize size{ CreatureSize::MEDIUM };
	// The Intelligence rating its Monstrous Manual entry prints, which the DMG reads its
	// Intelligence, Wisdom and Charisma by.
	IntelligenceRating intelligence{ IntelligenceRating::AVERAGE };

	// Ability scores
	// The Strength its Monstrous Manual entry prints, or none, in which case it is rolled
	// on Table 6 by size when the monster is built.
	std::optional<MonsterStrength> strength{};

	// Damage this creature deals
	DamageInfo damage{};

	// One item this creature starts wearing or wielding. The key names an entry
	// in items.json and is snake_case like every other data identifier.
	struct StartingItem
	{
		// The slot it starts in.
		EquipmentSlot slot{ EquipmentSlot::NONE };
		// The items.json key of what is in it.
		std::string itemKey{};
	};

	// What the creature carries into the dungeon. Empty for anything that
	// fights with its body.
	std::vector<StartingItem> equipment{};

	// What the creature strikes with when no slot holds a weapon - claws, a
	// bite, a gaze. Empty for anything that wields an item.
	std::string naturalAttack{};

	// Which body template this creature is built on, named in the body_plans
	// table: "beast" for anything with no slots, which is also what a monster
	// built in code starts as. The slots themselves come from the BodyPlanRegistry.
	std::string bodyPlanName{ "beast" };

	// Behaviour
	// The Ai it is built with, one a record may name.
	AiType aiType{ AiType::MONSTER };
	// Whether it can cross deep water.
	bool canSwim{ false };
	// Whether it crosses ground by flying over it, which is a second movement rate in
	// the Monstrous Manual: "Movement: 1, Fl 24 (B)" for a bat. A flier over water is
	// not in it and holds no breath there.
	bool canFly{ false };
	// Whether it crosses webs without being caught, its own or another's.
	bool walksWebs{ false };
	// How it feels about the player.
	Attitude attitude{ Attitude::HOSTILE };
	// Whether the player may swap places with it.
	bool displaceable{ true };

	// Spellcasting
	// The caster class and level it memorizes at, or none for a monster that memorizes nothing.
	std::optional<SpellcasterTraining> spellcasterTraining{};
	// The spell-like powers it uses without memorizing, in the order it tries them.
	std::vector<InnatePower> innatePowers{};

	// Spawn table
	int baseWeight{ 10 };
	// The first dungeon level it can spawn on.
	int levelMinimum{ 1 };
	// The last dungeon level it can spawn on; 0 means no limit.
	int levelMaximum{ 0 };
	// How much its spawn weight grows per level past the first; negative shrinks it.
	float levelScaling{ 0.0f };
};

// The standard and custom monsters; see the file comment above for use.
class MonsterRegistry
{
private:
	// Every standard monster's full parameters, by id.
	std::unordered_map<MonsterId, MonsterParams> standardMonsters{};
	// User-created monsters by key.
	std::map<std::string, MonsterParams, std::less<>> customMonsters{};

public:
	// Reads monster data, replacing everything held. A file missing a standard
	// monster's record throws.
	void load(std::string_view path);

	// Writes every standard monster and every custom monster.
	void save(std::string_view path) const;

	// Throws std::runtime_error naming the monster and the key when an innate power names a
	// spell the registry does not hold. Run after both registries load, since monsters.json
	// is read without the spells.
	//
	// Example:
	//   monsters.refuse_unknown_spells(spells);   // nothing, every power names a real spell
	//   // a pit fiend's power renamed "hellfire" -> throws, "pit_fiend ... 'hellfire'"
	void refuse_unknown_spells(const SpellRegistry& spells) const;

	// Every standard monster's parameters, by id - what the spawn table is drawn from.
	[[nodiscard]] const std::unordered_map<MonsterId, MonsterParams>& get_standard_monsters() const;

	// A standard monster's tile; no tile for one never loaded.
	[[nodiscard]] TileRef get_tile(MonsterId id) const;

	// Gives a standard monster a new tile.
	void set_tile(MonsterId id, TileRef tile);

	// Standard keys in their fixed order, then custom keys in key order.
	[[nodiscard]] std::vector<std::string> get_all_keys() const;

	// Throws std::out_of_range if key is unknown, or names a monster not loaded.
	[[nodiscard]] const MonsterParams& get_params(std::string_view key) const;

	// Replaces a standard or custom monster's parameters. Throws
	// std::out_of_range if key is unknown.
	void set_params(std::string_view key, const MonsterParams& params);

	// A monster's tile by key; no tile for an unknown key.
	[[nodiscard]] TileRef get_tile(std::string_view key) const;

	// Throws std::out_of_range if key is unknown.
	void set_tile(std::string_view key, TileRef tile);

	// Adds a user-created monster under a key made from its name, numbered past any key
	// already taken, and returns that key.
	[[nodiscard]] std::string add_custom(MonsterParams params);

	// Removes a user-created monster. Throws std::invalid_argument for a standard key
	// and std::out_of_range for an unknown one.
	void remove_custom(std::string_view key);

	// Whether a key names a standard monster.
	[[nodiscard]] bool is_builtin(std::string_view key) const;
};
