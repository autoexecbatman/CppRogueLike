#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

// Forward declarations
class Player;
class Creature;
struct GameContext;

// Mirror of Player::PlayerClassState to avoid circular dependency
enum class CasterClass
{
	CLERIC,
	WIZARD,
	NONE
};

enum class SpellClass
{
	CLERIC,
	WIZARD,
	BOTH
};

// Which builtin effect implementation a spell uses.
// Builtins have this set at initialisation. Custom spells pick one in SpellEditor.
enum class SpellEffectType
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
	KNOCK,
	NONE
};

struct SpellDefinition
{
	std::string name{};
	int level{};
	SpellClass spellClass{};
	std::string description{};
	SpellEffectType effect_type{ SpellEffectType::NONE };
};

class SpellSystem
{
public:
	// Get spell slots for class/level (AD&D 2e tables)
	static std::vector<int> get_spell_slots(CasterClass classState, int level);

	// Get all spell keys available to a class at given level.
	// Includes builtin and custom spells. Returns string keys.
	static std::vector<std::string> get_available_spells(CasterClass classState, int maxSpellLevel);

	// Cast a spell by string key (works for builtin and custom spells).
	// onSuccess is called when the spell takes effect (immediately for instant spells,
	// deferred via TargetingMenu callback for targeted spells).
	static void cast_spell_by_key(
		std::string_view key,
		Creature& caster,
		std::function<void(GameContext&)> onSuccess,
		GameContext& ctx);

	// Load / save spell metadata from JSON (call before Game construction).
	static void load(std::string_view path);
	static void save(std::string_view path);

	// --- Dynamic (string-keyed) API for editor use ---

	// Ordered: builtin keys (SPELL_KEYS order), then custom keys (alpha).
	static std::vector<std::string> get_all_keys();

	// Throws std::out_of_range if key is unknown.
	static const SpellDefinition& get_by_key(std::string_view key);

	// Updates builtin or custom spell by string key.
	static void set_by_key(std::string_view key, const SpellDefinition& def);

	// Add a new user-created spell. Returns the actual key used (derived from def.name).
	static std::string add_custom(SpellDefinition def);

	// Remove a user-created spell. Throws if key is builtin.
	static void remove_custom(std::string_view key);

	static bool is_builtin_key(std::string_view key);

	// Item-granted spells
	struct ItemGrantedSpell
	{
		std::string key{};
		std::string source{}; // "Ring", "Helm", etc.
	};
	static std::vector<ItemGrantedSpell> get_item_granted_spells(const Player& player);

	// Memorization
	static void show_memorization_menu(Player& player, GameContext& ctx);
	static void show_casting_menu(Player& player, GameContext& ctx);

private:
	// Dispatch helpers
	static void dispatch_effect(
		SpellEffectType effect,
		Creature& caster,
		std::function<void(GameContext&)> onSuccess,
		GameContext& ctx);

	// Spell effect implementations (instant — return true on success)
	static bool cast_cure_light_wounds(Creature& caster, GameContext& ctx);
	static bool cast_bless(Creature& caster, GameContext& ctx);
	static bool cast_sanctuary(Creature& caster, GameContext& ctx);
	static bool cast_magic_missile(Creature& caster, GameContext& ctx);
	static bool cast_shield(Creature& caster, GameContext& ctx);
	static bool cast_sleep(Creature& caster, GameContext& ctx);
	static bool cast_invisibility(Creature& caster, GameContext& ctx);
	static bool cast_teleport(Creature& caster, GameContext& ctx);
	static bool cast_knock(Creature& caster, GameContext& ctx);
	static bool cast_hold_person(Creature& caster, GameContext& ctx);

	// Targeted spell implementations — async via TargetingMenu; onSuccess fires on confirm
	static void cast_silence(Creature& caster, std::function<void(GameContext&)> onSuccess, GameContext& ctx);
	static void cast_web(Creature& caster, std::function<void(GameContext&)> onSuccess, GameContext& ctx);
	static void cast_fireball(Creature& caster, std::function<void(GameContext&)> onSuccess, GameContext& ctx);
};
