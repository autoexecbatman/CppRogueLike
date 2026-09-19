#pragma once

#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Alignment.h"
#include "DamageInfo.h"
#include "DiceExpr.h"
#include "EquipmentSlot.h"
#include "Renderer.h"

// file: MonsterRegistry.h
//
// Every monster the game knows, by key: the standard monsters with their full
// parameters, the class-based creatures built in code (mimic, shopkeeper, spiders),
// which hold only a tile here, and the custom monsters the monster editor adds. Game
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

// The monsters with a compiled-in identity: the standard monsters, then the
// class-based creatures that are constructed by their own classes.
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
	// Class-based creatures -- constructed via dedicated ctors, not MonsterCreator::create()
	MIMIC,
	SHOPKEEPER,
	SPIDER_SMALL,
	SPIDER_GIANT,
	SPIDER_WEAVER,
};

// Which AI a monster built from params gets.
enum class MonsterAiType
{
	MELEE,
	RANGED,
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
	int color{ 0 };
	// What its corpse is called.
	std::string corpseName;

	// Combat
	DiceExpr hpDice{};
	// The d20 roll it needs to hit armour class 0.
	int thaco{ 20 };
	// Its armour class; lower is harder to hit.
	int ac{ 10 };
	// The experience a kill is worth.
	int xp{ 0 };
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

	// Ability scores
	DiceExpr strDice{ 3, 6, 0 };
	// Dexterity, rolled when the monster is built.
	DiceExpr dexDice{ 3, 6, 0 };
	// Constitution, rolled when the monster is built.
	DiceExpr conDice{ 3, 6, 0 };
	// Intelligence, rolled when the monster is built.
	DiceExpr intDice{ 3, 6, 0 };
	// Wisdom, rolled when the monster is built.
	DiceExpr wisDice{ 3, 6, 0 };
	// Charisma, rolled when the monster is built.
	DiceExpr chaDice{ 3, 6, 0 };

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
	// table. Empty for anything that wears nothing, which is most of the
	// bestiary. The slots themselves come from the BodyPlanRegistry.
	std::string bodyPlanName{};

	// Behaviour
	MonsterAiType aiType{ MonsterAiType::MELEE };
	// Whether it can cross deep water.
	bool canSwim{ false };

	// Spawn table
	int baseWeight{ 10 };
	// The first dungeon level it can spawn on.
	int levelMinimum{ 1 };
	// The last dungeon level it can spawn on; 0 means no limit.
	int levelMaximum{ 0 };
	// How much its spawn weight grows per level past the first; negative shrinks it.
	float levelScaling{ 0.0f };
};

// The standard, class-based and custom monsters; see the file comment above for use.
class MonsterRegistry
{
private:
	// Every standard monster's full parameters, by id.
	std::unordered_map<MonsterId, MonsterParams> standardMonsters{};
	// The tile of each class-based creature the file gave one.
	std::unordered_map<MonsterId, TileRef> classTiles{};
	// Each class-based creature's name and tile, as the editor shows them.
	std::unordered_map<MonsterId, MonsterParams> classParams{};
	// User-created monsters by key.
	std::map<std::string, MonsterParams, std::less<>> customMonsters{};

public:
	// Reads monster data, replacing everything held. A file missing a standard
	// monster's record throws.
	void load(std::string_view path);

	// Writes every standard monster, every class-based creature's tile, and every
	// custom monster.
	void save(std::string_view path) const;

	// Every standard monster's parameters, by id - what the spawn table is drawn from.
	[[nodiscard]] const std::unordered_map<MonsterId, MonsterParams>& get_standard_monsters() const;

	// A standard monster's or class-based creature's tile; no tile for one never loaded.
	[[nodiscard]] TileRef get_tile(MonsterId id) const;

	// Gives a standard monster or class-based creature a new tile.
	void set_tile(MonsterId id, TileRef tile);

	// Standard keys in their fixed order, then custom keys in key order, then
	// class-based keys.
	[[nodiscard]] std::vector<std::string> get_all_keys() const;

	// Throws std::out_of_range if key is unknown, or names a monster not loaded.
	[[nodiscard]] const MonsterParams& get_params(std::string_view key) const;

	// Replaces a standard, class-based or custom monster's parameters. Throws
	// std::out_of_range if key is unknown.
	void set_params(std::string_view key, const MonsterParams& params);

	// A monster's tile by key; no tile for an unknown key.
	[[nodiscard]] TileRef get_tile(std::string_view key) const;

	// Throws std::out_of_range if key is unknown.
	void set_tile(std::string_view key, TileRef tile);

	// Adds a user-created monster under a key made from its name, numbered past any key
	// already taken, and returns that key.
	[[nodiscard]] std::string add_custom(MonsterParams params);

	// Removes a user-created monster. Throws std::invalid_argument for a standard or
	// class-based key and std::out_of_range for an unknown one.
	void remove_custom(std::string_view key);

	// Whether a key names a standard monster.
	[[nodiscard]] bool is_builtin(std::string_view key) const;

	// Whether a key names a class-based creature.
	[[nodiscard]] bool is_class_key(std::string_view key) const;
};
