#pragma once

#include "../Random/RandomDice.h"
#include "../Map/Map.h"
#include "../Actor/Actor.h"
#include "../Objects/Web.h"
#include "../Systems/SpellSystem.h"

class Item;
struct GameContext;

enum class EquipmentSlot
{
	HEAD,           // Helmets, hats
	NECK,           // Amulets, necklaces
	BODY,           // Armor (chain mail, plate mail, etc.)
	GIRDLE,         // Belts
	CLOAK,          // Cloaks, robes
	RIGHT_HAND,     // Main weapon
	LEFT_HAND,      // Shield or off-hand weapon
	RIGHT_RING,     // Rings
	LEFT_RING,      // Rings
	BRACERS,        // Bracers
	GAUNTLETS,      // Gloves, gauntlets
	BOOTS,          // Boots, shoes
	MISSILE_WEAPON, // Bows, crossbows
	MISSILES,       // Arrows, bolts
	TOOL,           // Tools, instruments
	NONE
};

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
	float attacksPerRound{ 1.0f }; // Tracks extra attacks (1.0 = 1 attack, 1.5 = 3/2 attacks, 2.0 = 2 attacks)
	int roundCounter{ 0 }; // Tracks rounds for alternating attack patterns
	std::vector<SpellId> memorizedSpells;

	// Equipment system
	std::vector<EquippedItem> equippedItems;

	Player(Vector2D position);
	void roll_new_character(GameContext& ctx);

	// Serialization - overrides Creature
	void load(const json& j) override;
	void save(json& j) override;

	// NOTE: coordinates are being set in the function create_room() in Map.cpp

	void racial_ability_adjustments();
	void equip_class_starting_gear(GameContext& ctx);
	void calculate_thaco();

	void consume_food(int nutrition, GameContext& ctx);

	void render(const GameContext& ctx) const noexcept;
	bool rest(GameContext& ctx);
	void animate_resting(GameContext& ctx);

	// Web effect tracking
	int webStuckTurns{ 0 };       // How many turns the player is stuck in a web
	int webStrength{ 0 };         // How strong the web is (affects escape difficulty)
	Web* trappingWeb{ nullptr };  // The web that has trapped the player

	bool is_webbed() const noexcept { return webStuckTurns > 0; } // Check if player is stuck in a web
	bool try_break_web(GameContext& ctx); // Attempt to break free from a web
	void get_stuck_in_web(int duration, int strength, Web* web, GameContext& ctx); // Get stuck in a web

	// Equipment system methods
	bool can_equip(const Item& item, EquipmentSlot slot) const noexcept;
	bool equip_item(std::unique_ptr<Item> item, EquipmentSlot slot, GameContext& ctx);
	bool unequip_item(EquipmentSlot slot, GameContext& ctx);
	Item* get_equipped_item(EquipmentSlot slot) const noexcept;
	bool is_slot_occupied(EquipmentSlot slot) const noexcept;
	bool is_dual_wielding() const noexcept;
	std::string get_equipped_weapon_damage_roll() const noexcept;
	
	// Equipment system - unique ID based methods
	bool toggle_armor(uint64_t item_unique_id, GameContext& ctx);
	bool is_item_equipped(uint64_t item_unique_id) const noexcept;
	bool toggle_weapon(uint64_t item_unique_id, EquipmentSlot preferred_slot, GameContext& ctx);
	bool toggle_shield(uint64_t item_unique_id, GameContext& ctx);
	bool toggle_equipment(uint64_t item_unique_id, EquipmentSlot slot, GameContext& ctx);

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
};
