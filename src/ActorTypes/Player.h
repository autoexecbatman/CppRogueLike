#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../Actor/Creature.h"
#include "../Actor/EquipmentSlot.h"
#include "../Ai/PlayerController.h"
#include "../Objects/Web.h"
#include "../Persistent/Persistent.h"
#include "../Utils/Vector2D.h"

class Item;
struct GameContext;
struct PlayerBlueprint;

struct EquippedItem
{
	std::unique_ptr<Item> item;
	EquipmentSlot slot;

	EquippedItem(std::unique_ptr<Item> i, EquipmentSlot s)
		: item(std::move(i)), slot(s) {}
};

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
	std::vector<EquippedItem> equippedItems;

	std::unique_ptr<PlayerController> controller;

	Player(Vector2D position);
	Player(Vector2D position, const PlayerBlueprint& blueprint, GameContext& ctx);
	void roll_new_character(GameContext& ctx);

	// Serialization - overrides Creature
	void load(const json& j) override;
	void save(json& j) override;

	// AD&D 2e: Class-specific Constitution HP multiplier caps
	int get_constitution_hp_multiplier() const noexcept override;

	// NOTE: coordinates are being set in the function create_room() in Map.cpp

	void racial_ability_adjustments(GameContext& ctx);
	void equip_class_starting_gear(GameContext& ctx);
	void calculate_thaco();

	void consume_food(int nutrition, GameContext& ctx);

	void render(const GameContext& ctx) const noexcept;
	bool rest(GameContext& ctx);
	void animate_resting(GameContext& ctx);
	bool attempt_hide(GameContext& ctx);

	// Web effect tracking
	int webStuckTurns{ 0 }; // How many turns the player is stuck in a web
	int webStrength{ 0 }; // How strong the web is (affects escape difficulty)
	Web* trappingWeb{ nullptr }; // The web that has trapped the player

	bool is_webbed() const noexcept { return webStuckTurns > 0; } // Check if player is stuck in a web
	bool try_break_web(GameContext& ctx); // Attempt to break free from a web
	void apply_web_effect(int duration, int strength, Web* web, GameContext& ctx) override; // Get stuck in a web

	// Equipment system methods
	bool can_equip(const Item& item, EquipmentSlot slot) const noexcept;
	bool equip_item(std::unique_ptr<Item> item, EquipmentSlot slot, GameContext& ctx);
	bool unequip_item(EquipmentSlot slot, GameContext& ctx);
	Item* get_equipped_item(EquipmentSlot slot) const noexcept override;
	bool is_slot_occupied(EquipmentSlot slot) const noexcept override;
	bool is_dual_wielding() const noexcept;

	// Equipment system - unique ID based methods
	bool toggle_armor(uint64_t itemUniqueId, GameContext& ctx) override;
	bool is_item_equipped(uint64_t itemUniqueId) const noexcept override;
	bool toggle_weapon(uint64_t itemUniqueId, EquipmentSlot preferredSlot, GameContext& ctx) override;
	bool toggle_shield(uint64_t itemUniqueId, GameContext& ctx) override;
	bool toggle_equipment(uint64_t itemUniqueId, EquipmentSlot slot, GameContext& ctx) override;

	bool is_player() const noexcept override { return true; }

	// AD&D 2e Open Locks: Rogue-only, level-based percentage chance
	int get_open_locks_skill() const noexcept override;

	// Display interface overrides
	std::string get_class_display_name() const override { return playerClass; }
	std::string get_race_display_name() const override { return playerRace; }
	int get_kill_count() const noexcept override { return killCount; }
	std::string get_equipped_weapon_damage_roll() const noexcept override;

	// Lifecycle hook overrides
	void on_new_game_start(GameContext& ctx) override;
	void recalculate_combat_stats() override;
	void die(GameContext& ctx) override;
	void on_kill_reward(int xp, GameContext& ctx) override;

	void update(GameContext& ctx) override;
	void apply_confusion(int duration) override;

	[[nodiscard]] int get_next_level_xp(GameContext& ctx) const;
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

private:
	// Helper for removing stat bonuses when unequipping stat-boost equipment
	void remove_stat_bonuses_from_equipment(Item& item);
	
	// Helper for applying stat bonuses when equipping stat-boost equipment
	void add_stat_bonuses_from_equipment(Item& item);
};
