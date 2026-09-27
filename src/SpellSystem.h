#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

class DataManager;

#include "SpellRegistry.h"
#include "Vector2D.h"

// Forward declarations
class Player;
class Creature;
struct GameContext;

class SpellSystem
{
public:
	// Get spell slots for class/level (AD&D 2e tables)
	// The progression table as the book prints it: Table 21 for a wizard, Table 24
	// for a priest, one row per level to 20th. Nothing about the caster beyond its
	// class and level, which is what makes it checkable against the page.
	static std::vector<int> progression_slots(CasterClass classState, int level);

	// The bonus spells a priest's Wisdom is worth, as a count per spell level
	// (Table 5). The column is a list and it is cumulative - a Wisdom of 15 carries
	// rows 13, 14 and 15 - so the rule is here and the rows are in wisdom.json.
	//
	// Example:
	//   bonus_priest_spells(15, dataManager);   // -> { 2, 1 }, two 1st and one 2nd
	//   bonus_priest_spells(12, dataManager);   // -> { }, nothing below 13
	static std::vector<int> bonus_priest_spells(int wisdom, const DataManager& dataManager);

	// What this caster actually memorises: the progression table, plus a priest's
	// Wisdom bonus where there is a row for it, and without the sixth and seventh
	// rows unless Table 24's footnotes are satisfied - Wisdom 17 and 18.
	static std::vector<int> get_spell_slots(CasterClass classState, int level, int wisdom, const DataManager& dataManager);

	// The highest spell level this caster can reach at that experience level, which
	// is how many rows of slots the table hands it. Everything that tells the player
	// what it may cast asks this rather than working it out again, the same way the
	// cleric's turning reach is asked rather than recomputed.
	//
	// Example:
	//   highest_spell_level(CasterClass::WIZARD, 11);   // -> 5, 11th grants no new level
	//   highest_spell_level(CasterClass::WIZARD, 12);   // -> 6
	//   highest_spell_level(CasterClass::NONE, 10);     // -> 0
	[[nodiscard]] static int highest_spell_level(CasterClass classState, int level, int wisdom, const DataManager& dataManager);

	// Cast a spell by string key (works for builtin and custom spells), read from
	// ctx.spellRegistry. onSuccess is called when the spell takes effect:
	// immediately for instant spells, and a turn later through the TargetingMenu
	// callback for targeted ones. It is required - a spell that lands always has a
	// turn to end.
	static void cast_spell_by_key(
		std::string_view key,
		Creature& caster,
		std::function<void(GameContext&)> onSuccess,
		GameContext& ctx);

	// Item-granted spells
	struct ItemGrantedSpell
	{
		std::string key{};
		std::string source{}; // "Ring", "Helm", etc.
	};
	static std::vector<ItemGrantedSpell> get_item_granted_spells(const Player& player);

	// What one fireball did: the dice it rolled and how many it struck.
	struct FireballBurst
	{
		int diceCount{ 0 };
		std::vector<int> dice{};
		int totalDamage{ 0 };
		int struck{ 0 };
	};

	// Burns every living creature within radius of center for 1d6 per caster
	// level, ten dice at most, rolled once for the whole burst. Each creature
	// saves versus spells at 15 or better with its fire resistance's bonus added,
	// takes the dice reduced per die by that resistance, and half of that on a
	// save. Public so a test can reach the burst without driving the targeting
	// menu that precedes it in play, and so the scroll can cast it.
	//
	// Example, caster level 3, one goblin adjacent, dice forced to 6, 6, 6 and a save of 1:
	//   burst_fireball(center, 3, 2, ctx);   // -> { 3, 18, 1 }, goblin takes 18
	static FireballBurst burst_fireball(Vector2D center, int casterLevel, int radius, GameContext& ctx);

	// Burns one creature with an already-rolled burst: its save with its fire
	// resistance's bonus, the dice reduced per die, half on a save.
	static void burn_with_fireball(Creature& target, const FireballBurst& burst, GameContext& ctx);

	// The level a spell read from a scroll is cast at. Dungeon Master's Guide:
	// "typically one level higher than that required to cast the spell, but
	// never below 6th level of experience". Fireball needs a 5th-level wizard,
	// so both readings give 6.
	static constexpr int SCROLL_FIREBALL_CASTER_LEVEL = 6;

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
	static bool cast_protection_from_evil(Creature& caster, GameContext& ctx);
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
