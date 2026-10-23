#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

class DataManager;
class RandomDice;

// Where a casting comes from. A priest's own memorised spell is the one its Wisdom
// can fail (Table 5); a spell a ring or a helm casts is the item's, not the priest's.
enum class SpellSource
{
	MEMORIZED,
	ITEM,
	// A monster's spell-like power, which nothing memorizes and Wisdom cannot fail.
	INNATE,
};

#include "SpellRegistry.h"
#include "Vector2D.h"

// Forward declarations
class Player;
class Creature;
class RandomDice;
struct GameContext;

// The rules a spell is made of, kept apart from the casting in SpellSystem, so a test can
// reach a rule without building a caster and a context around it.
namespace Spells
{

// Sleep affects 2d4 Hit Dice of monsters (Player's Handbook, PDF page 279).
inline constexpr int SLEEP_HIT_DICE_COUNT = 2;
inline constexpr int SLEEP_HIT_DICE_SIDES = 4;

// How many Hit Dice of monsters one casting of sleep affects. Each die is rolled on
// its own and the results summed, so the budget runs from 2 to 8 and sits on 5 more
// often than on either end. Rolling the count and the size as a range instead gives a
// flat 2 to 4, which can never reach a fifth Hit Die.
//
// Example, with two fours on the dice:
//   Spells::roll_sleep_hit_dice_budget(dice);   // -> 8, the most one casting can reach
[[nodiscard]] int roll_sleep_hit_dice_budget(RandomDice& dice);

} // namespace Spells

namespace SpellSystem
{

// Get spell slots for class/level (AD&D 2e tables)
// The progression table as the book prints it: Table 21 for a wizard, Table 24
// for a priest, one row per level to 20th. Nothing about the caster beyond its
// class and level, which is what makes it checkable against the page.
std::vector<int> progression_slots(CasterClass classState, int level);

// The bonus spells a priest's Wisdom is worth, as a count per spell level
// (Table 5). The column is a list and it is cumulative - a Wisdom of 15 carries
// rows 13, 14 and 15 - so the rule is here and the rows are in wisdom.json.
//
// Example:
//   bonus_priest_spells(15, dataManager);   // -> { 2, 1 }, two 1st and one 2nd
//   bonus_priest_spells(12, dataManager);   // -> { }, nothing below 13
std::vector<int> bonus_priest_spells(
	int wisdom,
	const DataManager& dataManager);

// What this caster actually memorises: the progression table, plus a priest's
// Wisdom bonus where there is a row for it, and without the sixth and seventh
// rows unless Table 24's footnotes are satisfied - Wisdom 17 and 18.
std::vector<int> get_spell_slots(
	CasterClass classState,
	int level,
	int wisdom,
	const DataManager& dataManager);

// The highest spell level this caster can reach at that experience level, which
// is how many rows of slots the table hands it. Everything that tells the player
// what it may cast asks this rather than working it out again, the same way the
// cleric's turning reach is asked rather than recomputed.
//
// Example:
//   highest_spell_level(CasterClass::WIZARD, 11);   // -> 5, 11th grants no new level
//   highest_spell_level(CasterClass::WIZARD, 12);   // -> 6
//   highest_spell_level(CasterClass::NONE, 10);     // -> 0
[[nodiscard]] int highest_spell_level(
	CasterClass classState,
	int level,
	int wisdom,
	const DataManager& dataManager);

// Cast a spell by string key (works for builtin and custom spells), read from
// ctx.spellRegistry. onSuccess is called when the spell takes effect:
// immediately for instant spells, and a turn later through the TargetingMenu
// callback for targeted ones. It is required - a spell that lands always has a
// turn to end.
void cast_spell_by_key(
	std::string_view key,
	Creature& caster,
	SpellSource source,
	std::function<void(GameContext&)> onCastComplete,
	GameContext& ctx);

// Casts as cast_spell_by_key does, the same refusals and fizzle included, at a target the
// caller names rather than one the player chooses: magic missile strikes the target, and a
// fireball bursts on its tile. Asserts can_aim_at says the spell can be aimed. The dead are
// left for the caller's sweep, because a monster casts from inside the creature loop.
//
// Example, a lich of wizard 18 four tiles from the player:
//   cast_spell_at("magic_missile", lich, SpellSource::MEMORIZED, player, on_spent, ctx);
//   // -> five missiles at the player; on_spent runs
void cast_spell_at(
	std::string_view key,
	Creature& caster,
	SpellSource source,
	Creature& target,
	std::function<void(GameContext&)> onCastComplete,
	GameContext& ctx);

// Whether the spell can be aimed at target from where the caster stands: magic missile
// needs a line of sight, and a fireball needs one, its range of ten tiles plus the caster
// level, and the target outside its burst radius, so the caster stands clear of it. Every
// other spell answers false, since none of them can be aimed yet.
//
// Example, a lich of wizard 18:
//   can_aim_at(SpellEffectType::FIREBALL, lich, playerFourTilesAway, ctx);   // -> true
//   can_aim_at(SpellEffectType::FIREBALL, lich, playerTwoTilesAway, ctx);    // -> false
//   can_aim_at(SpellEffectType::SLEEP, lich, playerFourTilesAway, ctx);      // -> false
[[nodiscard]] bool can_aim_at(
	SpellEffectType effect,
	const Creature& caster,
	const Creature& target,
	GameContext& ctx);

// The spell list a creature casts from: its record's training when it has one, otherwise
// its class's.
//
// Example:
//   caster_class(wizardPlayer);   // -> CasterClass::WIZARD
//   caster_class(lich);           // -> CasterClass::WIZARD, from its training
//   caster_class(goblin);         // -> CasterClass::NONE
[[nodiscard]] CasterClass caster_class(const Creature& caster);

// The level a creature casts at: its record's training level when it has one, otherwise its
// creature level, which for a monster's innate power is its Hit Dice.
//
// Example:
//   caster_level(lich);       // -> 18, its training, where its Hit Dice are 11
//   caster_level(pitFiend);   // -> 13, its Hit Dice
[[nodiscard]] int caster_level(const Creature& caster);

// The spells a caster of that class, level and Wisdom memorizes: per spell level, lowest
// first, each spell the registry has for the class once, in the registry's order, while
// slots remain. Empty for CasterClass::NONE.
//
// Example:
//   spells_to_memorize(CasterClass::WIZARD, 1, 10, spells, dataManager);   // -> { "magic_missile" }
//   spells_to_memorize(CasterClass::NONE, 9, 10, spells, dataManager);     // -> { }
[[nodiscard]] std::vector<std::string> spells_to_memorize(
	CasterClass casterClass,
	int casterLevel,
	int wisdom,
	const SpellRegistry& spells,
	const DataManager& dataManager);

// Replaces what the creature has memorized with spells_to_memorize for its own class, level
// and Wisdom. The one fill, for the player's rest and creation and a monster's spawn alike.
//
// Example:
//   memorize_spells(lich, spells, dataManager);   // lich.spellcasting.memorizedSpells -> eight keys
void memorize_spells(
	Creature& caster,
	const SpellRegistry& spells,
	const DataManager& dataManager);

// What Table 5 gives this caster's Wisdom as a percentage chance that any one of
// its spells fizzles. Zero for anyone but a priest, and for a priest of Wisdom 13
// or better.
//
// Example:
//   spell_failure_chance(priestOfWisdomNine, dataManager);   // -> 20
[[nodiscard]] int spell_failure_chance(
	const Creature& caster,
	const DataManager& dataManager);

// Rolls percentile dice against that chance, and answers whether this casting is
// lost. A caster with no chance to fail is not made to roll.
[[nodiscard]] bool spell_fizzles(
	const Creature& caster,
	const DataManager& dataManager,
	RandomDice& dice);

// What choosing an item's entry in the cast menu does: cast the spell it grants, or
// speak the word that wakes it.
enum class ItemPower
{
	CAST_SPELL,
	// The worn helm's spell, whose casting spends the helm's use for the day.
	CAST_SPELL_ONCE_A_DAY,
	SPEAK_COMMAND_WORD
};

// Item-granted spells. A command word has no spell, so its key is empty.
struct ItemGrantedSpell
{
	std::string key{};
	std::string source{}; // "Ring", "Helm", etc.
	ItemPower power{ ItemPower::CAST_SPELL };
};

// What the player's worn items let them cast at game time `now`. A helm of teleportation
// is listed only while is_helm_ready says its day has passed; the ring is always listed.
std::vector<ItemGrantedSpell> get_item_granted_spells(
	const Player& player,
	int now);

// Whether a burst reached the player, so a caller animates the hit the burst dealt.
enum class PlayerReach
{
	OUTSIDE,
	STRUCK
};

// What one fireball did: the dice it rolled, how many it struck, and whether the player was one.
struct FireballBurst
{
	int diceCount{ 0 };
	std::vector<int> dice{};
	int totalDamage{ 0 };
	int struck{ 0 };
	PlayerReach playerReach{ PlayerReach::OUTSIDE };
};

// Burns every living creature within radius of center, the player among them,
// for 1d6 per caster level, ten dice at most, rolled once for the whole burst.
// Each creature saves versus spells on its own Table 60 row with its fire
// resistance's bonus added, takes the dice reduced per die by that resistance,
// and half of that on a save. Public so a test can reach the burst without
// driving the targeting menu that precedes it in play, and so the scroll can cast it.
//
// Example, caster level 3, one goblin adjacent, dice forced to 6, 6, 6 and a save of 1:
//   burst_fireball(center, 3, 2, ctx);   // -> { 3, 18, 1, OUTSIDE }, goblin takes 18
//   // the same with the player beside the goblin -> { 3, 18, 2, STRUCK }
FireballBurst burst_fireball(
	Vector2D center,
	int casterLevel,
	int radius,
	GameContext& ctx);

// Burns one creature with an already-rolled burst: its save with its fire
// resistance's bonus, the dice reduced per die, half on a save.
void burn_with_fireball(
	Creature& target,
	const FireballBurst& burst,
	GameContext& ctx);

// The level a spell read from a scroll is cast at. Dungeon Master's Guide:
// "typically one level higher than that required to cast the spell, but
// never below 6th level of experience". Fireball needs a 5th-level wizard,
// so both readings give 6.
inline constexpr int SCROLL_FIREBALL_CASTER_LEVEL = 6;

// Memorization
void show_memorization_menu(Player& player, GameContext& ctx);
void show_casting_menu(Player& player, GameContext& ctx);

} // namespace SpellSystem
