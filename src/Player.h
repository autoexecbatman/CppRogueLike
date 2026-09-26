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
#include "PlayerController.h"
#include "Persistent.h"
#include "ThiefSkills.h"
#include "Vector2D.h"

class Item;
struct GameContext;
struct PlayerBlueprint;

class Player : public Creature
{
public:
	enum class PlayerRaceState
	{
		NONE,
		HUMAN,
		ELF,
		DWARF,
		HALFLING,
		GNOME,
		HALFELF
	} playerRaceState{ PlayerRaceState::NONE };

	enum class PlayerClassState
	{
		NONE,
		FIGHTER,
		ROGUE,
		CLERIC,
		WIZARD
	} playerClassState{ PlayerClassState::NONE };

	std::string playerClass{ "None" };
	std::string playerRace{ "None" };
	int roundCounter{ 0 }; // Tracks rounds for alternating attack patterns
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
	void load(const json& j) override;
	void save(json& j) override;

	// NOTE: coordinates are being set in the function create_room() in Map.cpp

	// Pays the race onto the six scores and reports what it did, one line per
	// ability it moved. racial_ability_adjustments shows those lines; this is the
	// half that changes the character, and it needs no window to run.
	//
	// Example, a halfling:
	//   pay_racial_adjustments();   // -> { "-1 Strength", "+1 Dexterity" }
	[[nodiscard]] std::vector<std::string> pay_racial_adjustments();
	void racial_ability_adjustments(GameContext& ctx);
	void equip_class_starting_gear(GameContext& ctx);
	void calculate_thaco();

	void consume_food(int nutrition, GameContext& ctx);

	void render(const GameContext& ctx) const noexcept;
	bool rest(GameContext& ctx);
	void animate_resting(GameContext& ctx);
	bool attempt_hide(GameContext& ctx);

	// Equipment system methods
	bool equip_item(std::unique_ptr<Item> item, EquipmentSlot slot, GameContext& ctx);
	bool unequip_item(EquipmentSlot slot, GameContext& ctx);
	bool is_slot_occupied(EquipmentSlot slot) const noexcept;
	bool is_dual_wielding() const noexcept;

	// Equipment system - unique ID based methods
	bool toggle_armor(uint64_t itemUniqueId, GameContext& ctx);
	bool is_item_equipped(uint64_t itemUniqueId) const noexcept;
	bool toggle_weapon(uint64_t itemUniqueId, EquipmentSlot preferredSlot, GameContext& ctx);
	bool toggle_shield(uint64_t itemUniqueId, GameContext& ctx);
	bool toggle_equipment(uint64_t itemUniqueId, EquipmentSlot slot, GameContext& ctx);

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
	std::string get_class_display_name() const { return playerClass; }
	std::string get_race_display_name() const { return playerRace; }
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
	void apply_confusion(int duration) override;

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

// What a race does to the six scores, in ALL_ABILITY order. AD&D 2e racial ability
// adjustments; a human and a half-elf take none. One table, read both by the
// allocation screen, which shows what a score will become, and by
// Player::racial_ability_adjustments, which pays it once the character exists.
//
// Example:
//   racial_ability_modifiers(Player::PlayerRaceState::HALFLING);   // -> { -1, 1, 0, 0, 0, 0 }
//   racial_ability_modifiers(Player::PlayerRaceState::HUMAN);      // -> all zero
[[nodiscard]] std::array<int, ABILITY_COUNT> racial_ability_modifiers(Player::PlayerRaceState race);

// Player's Handbook Table 27, Thieving Skill Racial Adjustments (PDF page 85), in
// ALL_THIEF_SKILL order. A human takes none. It lives here rather than in
// ThiefSkills because the race does: the tables there take the column as a number.
//
// Example:
//   thief_skill_racial_adjustments(Player::PlayerRaceState::DWARF);
//   // -> { 0, 10, 15, 0, 0, 0, -10, -5 }, being Open Locks +10 and Climb Walls -10
[[nodiscard]] std::array<int, THIEF_SKILL_COUNT> thief_skill_racial_adjustments(Player::PlayerRaceState race);
