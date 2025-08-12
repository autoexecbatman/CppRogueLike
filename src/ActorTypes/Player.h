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
	MAIN_HAND,
	OFF_HAND,
	ARMOR,
	NONE
};

struct EquippedItem
{
	std::unique_ptr<Item> item;
	EquipmentSlot slot;
	bool twoHandedGrip{false}; // For versatile weapons used two-handed
	
	EquippedItem(std::unique_ptr<Item> i, EquipmentSlot s, bool twoHanded = false)
		: item(std::move(i)), slot(s), twoHandedGrip(twoHanded) {}
};

class Player : public Creature
{
public:
	enum class PlayerRaceState : int
	{
		NONE,
		HUMAN,
		ELF,
		DWARF,
		HALFLING,
		GNOME,
		HALFELF
	} playerRaceState{ PlayerRaceState::NONE };

	enum class PlayerClassState : int
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

	// Note::
	// X/Y coordinates set
	// in the function create_room()
	// in Map.cpp

	void setXY(Vector2D pos) noexcept { position = pos; }
	Vector2D getXY() const noexcept { return position; }

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

	// Check if player is stuck in a web
	bool isWebbed() const { return webStuckTurns > 0; }

	// Attempt to break free from a web
	bool tryBreakWeb();

	// Get stuck in a web
	void getStuckInWeb(int duration, int strength, Web* web);

	// Equipment system methods
	bool canEquip(const Item& item, EquipmentSlot slot, bool twoHanded = false) const;
	bool equipItem(std::unique_ptr<Item> item, EquipmentSlot slot, bool twoHanded = false);
	bool unequipItem(EquipmentSlot slot);
	Item* getEquippedItem(EquipmentSlot slot) const;
	bool isSlotOccupied(EquipmentSlot slot) const;
	bool hasEquippedTwoHandedWeapon() const;
	std::string getEquippedWeaponDamageRoll() const;
};

#endif // !PLAYER_H
// end of file: Player.h
