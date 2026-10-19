#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "AbilityAllocation.h"
#include "Creature.h"
#include "EquipmentSlot.h"
#include "Persistent.h"
#include "PlayerController.h"
#include "PlayerRaceState.h"
#include "ThiefSkills.h"
#include "Vector2D.h"

class Item;
struct GameContext;
struct PlayerBlueprint;

// How close a living hostile creature may be before resting is refused. Chebyshev
// distance, so it is an 11-by-11 square around the player rather than a circle.
inline constexpr int REST_DANGER_RADIUS = 5;

class Player : public Creature
{
private:
	// The square the thief last tried to hide on, whether the secret roll made it or
	// not: "the thief always thinks he is hidden" (PHB page 87). Empty out of hiding.
	std::optional<Vector2D> hidingSpot{};

public:
	PlayerRaceState playerRaceState{ PlayerRaceState::NONE };

	int killCount{ 0 }; // Tracks kill count for log.

	std::vector<std::string> memorizedSpells;

	// The discretionary percentage points the player has put on each thief skill,
	// in ALL_THIEF_SKILL order. Choices rather than a function of level, so they
	// are stored and saved; every other part of a skill's number is derived.
	std::array<int, THIEF_SKILL_COUNT> thiefSkillPoints{};

	std::unique_ptr<PlayerController> controller;

	Player(Vector2D position);
	Player(Vector2D position, const PlayerBlueprint& blueprint, GameContext& ctx);
	void roll_new_character(GameContext& ctx);

	// Tile config key for this character's sprite, e.g.
	// "TILE_PLAYER_DWARF_CLERIC". Races without art fall back to the human
	// row, and an unset race or class falls back to "TILE_PLAYER".
	[[nodiscard]] std::string_view sprite_tile_key() const noexcept;

	// Serialization - overrides Creature
	void load(const json& source) override;
	void save(json& output) override;

	// NOTE: coordinates are being set in the function create_room() in Map.cpp

	// Pays the race onto the six scores and reports what it did, one line per
	// ability it moved. show_racial_adjustments shows those lines; this is the
	// half that changes the character, and it needs no window to run.
	//
	// Example, a halfling:
	//   pay_racial_adjustments();   // -> { "-1 Strength", "+1 Dexterity" }
	[[nodiscard]] std::vector<std::string> pay_racial_adjustments();
	// Opens the popup naming what the race did to the scores. A race that moved none,
	// human or half-elf, opens nothing.
	//
	// Example, a halfling:
	//   show_racial_adjustments(pay_racial_adjustments(), ctx); // a popup listing -1 Strength, +1 Dexterity
	//   show_racial_adjustments({}, ctx); // a human: no popup
	void show_racial_adjustments(std::vector<std::string> bonusLines, GameContext& ctx);
	void equip_class_starting_gear(GameContext& ctx);
	void calculate_thaco();

	void consume_food(int nutrition, GameContext& ctx);

	void render(const GameContext& ctx) const noexcept;
	bool rest(GameContext& ctx);
	void animate_resting(GameContext& ctx);
	// What a try at hiding did. Success and failure are one answer on purpose:
	// "The DM rolls the dice and keeps the result secret, but the thief always
	// thinks he is hidden" (PHB, Hide in Shadows). A caller that could tell them
	// apart would leak the roll, so the type does not let it.
	enum class HideAttempt
	{
		NOT_A_THIEF, // no Hide in Shadows percentage to roll against
		ALREADY_HIDDEN, // nothing to try; the thief is already in the shadows
		WATCHED, // someone is looking, which no roll can beat
		ATTEMPTED, // the roll was made, and the thief believes it worked
	};

	// One try at melting into the shadows, rolled against Hide in Shadows and kept
	// secret from the player. The turn is spent on ATTEMPTED whatever the roll said.
	//
	// Example, a thief nobody has noticed:
	//   player.attempt_hide(ctx);   // -> HideAttempt::ATTEMPTED, hidden or not
	// The same thief with a monster hunting it:
	//   player.attempt_hide(ctx);   // -> HideAttempt::WATCHED, no roll made
	[[nodiscard]] HideAttempt attempt_hide(GameContext& ctx);

	// Whether the thief believes he is hidden: he tried, and has not moved or attacked
	// since. True after a failed roll as well, which is what keeps the roll secret.
	//
	// Example:
	//   player.attempt_hide(ctx);   // the roll fails, in secret
	//   player.is_in_shadows();     // -> true
	//   player.is_invisible();      // -> false
	[[nodiscard]] bool is_in_shadows() const noexcept;

	// Ends a hide, the secret roll's result with it, and says so in a line that reads
	// the same whether the roll made it or not. Refuses a thief not in the shadows.
	//
	// Example, a thief whose roll succeeded and one whose roll failed:
	//   player.leave_shadows(ctx);   // "You leave the shadows." for both
	//   player.is_in_shadows();      // -> false
	void leave_shadows(GameContext& ctx);

	// Ends the hide if the thief is no longer on the square he hid on, by whatever means
	// he left it: "he remains hidden only as long as he remains virtually motionless".
	// Run before any creature acts on the player's last action.
	//
	// Example, a thief who hid at (2, 5):
	//   player.leave_shadows_if_moved(ctx);   // still at (2, 5): still hidden
	//   player.position = Vector2D{ 3, 5 };
	//   player.leave_shadows_if_moved(ctx);   // "You leave the shadows."
	void leave_shadows_if_moved(GameContext& ctx);

	// Equipment system methods

	// Equipment system - unique ID based methods

	bool is_player() const noexcept override { return true; }

	// Which of Table 29's columns the armour on this character's body is read in.
	// An empty body slot is the "No Armor" column; armour heavier than chain mail
	// is nullopt, because the table prints no column for it and the book does not
	// let a thief wear it.
	//
	// Example, a rogue in the leather the starting kit gives:
	//   player.thief_armor();   // -> ThiefArmor::LEATHER
	[[nodiscard]] std::optional<ThiefArmor> thief_armor() const noexcept;

	// The percentage this character rolls against for a thief skill, Tables 26 to
	// 29 and the points spent all included. Nullopt for anyone who is not a rogue,
	// and for a rogue in armour Table 29 has no column for.
	//
	// Example, a 1st-level dwarf rogue with Dexterity 17 in leather, no points yet:
	//   player.thief_skill(ThiefSkill::OPEN_LOCKS);   // -> 30
	// The same rogue wearing plate mail:
	//   player.thief_skill(ThiefSkill::OPEN_LOCKS);   // -> nullopt
	[[nodiscard]] std::optional<int> thief_skill(ThiefSkill skill) const noexcept;

	// Display interface overrides
	std::string get_class_display_name() const;
	std::string get_race_display_name() const;

	int get_kill_count() const noexcept { return killCount; }

	std::string get_equipped_weapon_damage_roll() const noexcept;

	// Lifecycle hook overrides
	void on_new_game_start(GameContext& ctx);

	// PHB character creation: a fighter who is not a halfling and has Strength 18 rolls
	// d100 for exceptional Strength, a roll of 100 being 18/00. Anyone else keeps none.
	//
	// Example, a human fighter with Strength 18, the d100 showing 76:
	//   player.roll_exceptional_strength(ctx);   // get_exceptional_strength() -> 76
	// A halfling fighter, or a cleric, with the same 18 rolls nothing and keeps 0.
	void roll_exceptional_strength(GameContext& ctx);
	void recalculate_combat_stats();
	void die(GameContext& ctx) override;
	void on_kill_reward(int xp, GameContext& ctx);

	void update(GameContext& ctx) override;
	void apply_confusion(int durationRounds, int currentTime) override;

	[[nodiscard]] int get_next_level_xp() const;
	void levelup_update(GameContext& ctx);

	// Two-weapon fighting mechanics
	struct DualWieldInfo
	{
		bool isDualWielding{ false };
		int mainHandPenalty{ 0 };
		int offHandPenalty{ 0 };
		std::string offHandDamageRoll{ "D2" };
	};

	DualWieldInfo get_dual_wield_info() const noexcept;
};

// The six a character can be, in the order the race menu offers them. NONE is not
// among them: it is the state a Player has before a race is chosen, not a choice.
inline constexpr std::array<PlayerRaceState, 6> ALL_PLAYER_RACE{
	PlayerRaceState::HUMAN,
	PlayerRaceState::DWARF,
	PlayerRaceState::ELF,
	PlayerRaceState::GNOME,
	PlayerRaceState::HALFELF,
	PlayerRaceState::HALFLING
};

// The four classes a character can be, in the order the class menu offers them.
// MONSTER is not among them: it is the class of every creature that is not a player.
inline constexpr std::array<CreatureClass, 4> ALL_PLAYER_CLASS{
	CreatureClass::FIGHTER,
	CreatureClass::ROGUE,
	CreatureClass::CLERIC,
	CreatureClass::WIZARD
};

// What a class is called on screen. **This is the only place a class's name is
// written** - the class menu, the character sheet, the level-up screen, the HUD and
// the death screen all read it from here. MONSTER is never a player's class, so asking
// for its name is an assertion rather than an answer.
//
// Example:
//   class_display_name(CreatureClass::CLERIC);    // -> "Cleric"
//   class_display_name(CreatureClass::MONSTER);   // -> asserts
[[nodiscard]] std::string_view class_display_name(CreatureClass creatureClass);

// What a race is called on screen. **This is the only place a race's name is
// written** - the menu that offers it, the character sheet and the death screen all
// read it from here, so no two of them can disagree. NONE answers "None", which is
// what an unchosen race prints.
//
// Example:
//   race_display_name(PlayerRaceState::HALFELF);   // -> "Half-Elf"
//   race_display_name(PlayerRaceState::NONE);      // -> "None"
[[nodiscard]] std::string_view race_display_name(PlayerRaceState race);

// What a race does to the six scores, in ALL_ABILITY order. AD&D 2e racial ability
// adjustments; a human and a half-elf take none. One table, read both by the
// allocation screen, which shows what a score will become, and by
// Player::pay_racial_adjustments, which pays it once the character exists.
//
// Example:
//   racial_ability_modifiers(PlayerRaceState::HALFLING);   // -> { -1, 1, 0, 0, 0, 0 }
//   racial_ability_modifiers(PlayerRaceState::HUMAN);      // -> all zero
[[nodiscard]] std::array<int, ABILITY_COUNT> racial_ability_modifiers(PlayerRaceState race);

// Player's Handbook Table 27, Thieving Skill Racial Adjustments (PDF page 85), in
// ALL_THIEF_SKILL order. A human takes none. It lives here rather than in
// ThiefSkills because the race does: the tables there take the column as a number.
//
// Example:
//   thief_skill_racial_adjustments(PlayerRaceState::DWARF);
//   // -> { 0, 10, 15, 0, 0, 0, -10, -5 }, being Open Locks +10 and Climb Walls -10
[[nodiscard]] std::array<int, THIEF_SKILL_COUNT> thief_skill_racial_adjustments(PlayerRaceState race);
