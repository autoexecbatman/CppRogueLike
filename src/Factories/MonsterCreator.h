#pragma once
// file: MonsterCreator.h

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "../Combat/DamageInfo.h"
#include "../Renderer/Renderer.h"

// Forward declarations
class Creature;
struct Vector2D;
struct GameContext;

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

enum class MonsterAiType
{
	MELEE,
	RANGED,
};

// Dice expression: roll num dice of sides sides, add bonus. num=0 means skip.
struct DiceExpr
{
	int num{ 0 };
	int sides{ 0 };
	int bonus{ 0 };
};

struct MonsterParams
{
	// Identity
	TileRef symbol{};
	std::string name;
	int color{ 0 };
	std::string corpseName;

	// Combat
	DiceExpr hpDice{};
	int thaco{ 20 };
	int ac{ 10 };
	int xp{ 0 };
	int dr{ 0 };
	int morale{ 10 };
	int corpseWeight{ 50 };

	// Ability scores
	DiceExpr strDice{ 3, 6, 0 };
	DiceExpr dexDice{ 3, 6, 0 };
	DiceExpr conDice{ 3, 6, 0 };
	DiceExpr intDice{ 3, 6, 0 };
	DiceExpr wisDice{ 3, 6, 0 };
	DiceExpr chaDice{ 3, 6, 0 };

	// Weapon display + damage
	std::string weaponName;
	DamageInfo damage{};

	// Behaviour
	MonsterAiType aiType{ MonsterAiType::MELEE };
	bool canSwim{ false };

	// Spawn table
	int baseWeight{ 10 };
	int levelMinimum{ 1 };
	int levelMaximum{ 0 };
	float levelScaling{ 0.0f };
};

namespace MonsterCreator
{
// Load all monster data from JSON. Must be called before Game construction.
void load(std::string_view path);

// Persist the current registry back to JSON (called by ContentEditor on close).
void save(std::string_view path);

// Standard-monster registry (goblin, orc, ...). Used by MonsterFactory.
[[nodiscard]] const std::unordered_map<MonsterId, MonsterParams>& get_registry();

// Tile access for all monsters including class-based ones (mimic, shopkeeper, spiders).
[[nodiscard]] TileRef get_tile(MonsterId id);
void set_tile(MonsterId id, TileRef tile);

// Overwrite full params for a standard monster. No-op for class-based.
void set_params(MonsterId id, const MonsterParams& p);

// Factory: create a standard monster at pos. Not for class-based creatures.
[[nodiscard]] std::unique_ptr<Creature> create(Vector2D pos, MonsterId id, GameContext& ctx);
[[nodiscard]] std::unique_ptr<Creature> create_from_params(Vector2D pos, const MonsterParams& params, GameContext& ctx);

// --- Dynamic (string-keyed) API for editor use ---

// Ordered: builtin standard keys, then custom keys (alpha), then class-based keys.
[[nodiscard]] std::vector<std::string> get_all_keys();

// Throws std::out_of_range if key is unknown.
[[nodiscard]] const MonsterParams& get_params(std::string_view key);

// Updates builtin, custom, or class-based entry by string key.
void set_params(std::string_view key, const MonsterParams& p);

// Tile access by string key.
[[nodiscard]] TileRef get_tile(std::string_view key);
void set_tile(std::string_view key, TileRef tile);

// Add a new user-created monster. Returns the actual key used (derived from p.name).
[[nodiscard]] std::string add_custom(MonsterParams p);

// Remove a user-created monster. Throws if key is builtin or class-based.
void remove_custom(std::string_view key);

[[nodiscard]] bool is_builtin(std::string_view key);
[[nodiscard]] bool is_class_key(std::string_view key);

} // namespace MonsterCreator
