// file: Player.h
#ifndef PLAYER_H
#define PLAYER_H

#include "../Random/RandomDice.h"
#include "../Map/Map.h"
#include "../Actor/Actor.h"
#include "../Objects/Web.h"

class Item;

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

	std::string playerGender{ "None" };
	std::string playerClass{ "None" };
	std::string playerRace{ "None" };
	int playerLevel{ 1 };
	float attacksPerRound{ 1.0f }; // Tracks extra attacks (1.0 = 1 attack, 1.5 = 3/2 attacks, 2.0 = 2 attacks)
	int roundCounter{ 0 }; // Tracks rounds for alternating attack patterns

	// Equipment system
	std::vector<EquippedItem> equippedItems;

	Player(Vector2D position);

	// NOTE: coordinates are being set in the function create_room() in Map.cpp

	void racial_ability_adjustments();
	void calculate_thaco();

	void consume_food(int nutrition);

	std::string get_weapon_equipped() const noexcept { return weaponEquipped; }
	void set_weapon_equipped(std::string weapon) noexcept { weaponEquipped = weapon; }

	void render() const noexcept;
	bool rest();
	void animate_resting();

	// Web effect tracking
	int webStuckTurns = 0;       // How many turns the player is stuck in a web
	int webStrength = 0;         // How strong the web is (affects escape difficulty)
	Web* trappingWeb = nullptr;  // The web that has trapped the player

	bool is_webbed() const { return webStuckTurns > 0; } // Check if player is stuck in a web
	bool try_break_web(); // Attempt to break free from a web
	void get_stuck_in_web(int duration, int strength, Web* web); // Get stuck in a web

	// Equipment system methods
	bool can_equip_item(const Item& item, EquipmentSlot slot) const;
	bool equip_item(std::unique_ptr<Item> item, EquipmentSlot slot);
	bool unequip_item(EquipmentSlot slot);
	Item* get_equipped_item_in_slot(EquipmentSlot slot) const;
	bool is_slot_occupied(EquipmentSlot slot) const;
	bool is_dual_wielding() const;
	std::string get_equipped_weapon_damage_roll() const;
	
	// Two-weapon fighting mechanics
	struct DualWieldInfo
	{
		bool isDualWielding = false;
		int mainHandPenalty = 0;
		int offHandPenalty = 0;
		std::string offHandDamageRoll = "D2";
	};
	DualWieldInfo get_dual_wield_info() const;
};

#endif // !PLAYER_H
// end of file: Player.h
