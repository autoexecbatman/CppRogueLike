// file: Pickable.cpp
#include <vector>

#include "Pickable.h"
#include "../Colors/Colors.h"
#include "../Game.h"
#include "Actor.h"
#include "Container.h"
#include "../ActorTypes/Player.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/Fireball.h"
#include "../ActorTypes/Confuser.h"
#include "../Ai/AiMonsterConfused.h"
#include "../ActorTypes/Gold.h"
#include "../Items/Food.h"
#include "../Items/CorpseFood.h"
#include "../Items/Amulet.h"
#include "../Items/Armor.h"

//==PICKABLE==
bool Pickable::use(Item& owner, Creature& wearer)
{
	if (wearer.container)
	{
		auto compareItems = [&owner](const std::unique_ptr<Item>& actor) { return actor.get() == &owner; };
		std::erase_if(wearer.container->get_inventory_mutable(), compareItems);

		return true;
	}

	return false;
}

// AD&D 2e weapon size and dual-wield validation methods
WeaponSize Weapon::get_weapon_size() const
{
	// Map specific weapon types to their AD&D 2e sizes
	if (dynamic_cast<const Dagger*>(this))
	{
		return WeaponSize::TINY;  // Dagger is TINY
	}
	else if (dynamic_cast<const ShortSword*>(this))
	{
		return WeaponSize::SMALL; // Short sword is SMALL
	}
	else if (dynamic_cast<const LongSword*>(this) || dynamic_cast<const BattleAxe*>(this) || dynamic_cast<const WarHammer*>(this))
	{
		return WeaponSize::MEDIUM; // Long sword, battle axe, war hammer are MEDIUM
	}
	else if (dynamic_cast<const Greatsword*>(this) || dynamic_cast<const GreatAxe*>(this))
	{
		return WeaponSize::LARGE; // Two-handed weapons are LARGE
	}
	else if (dynamic_cast<const Staff*>(this))
	{
		return WeaponSize::MEDIUM; // Staff is MEDIUM (versatile)
	}
	else
	{
		return WeaponSize::MEDIUM; // Default to MEDIUM
	}
}

bool Weapon::can_be_off_hand(WeaponSize weaponSize) const
{
	// AD&D 2e rule: Off-hand weapons must be SMALL or TINY
	return weaponSize <= WeaponSize::SMALL;
}

bool Weapon::validate_dual_wield(Item* mainHandWeapon, Item* offHandWeapon) const
{
	if (!mainHandWeapon || !offHandWeapon)
	{
		return false;
	}
	
	// Get weapon pickables
	auto* mainWeapon = dynamic_cast<Weapon*>(mainHandWeapon->pickable.get());
	auto* offWeapon = dynamic_cast<Weapon*>(offHandWeapon->pickable.get());
	
	if (!mainWeapon || !offWeapon)
	{
		return false; // One of them is not a weapon
	}
	
	// Get weapon sizes
	WeaponSize mainSize = mainWeapon->get_weapon_size();
	WeaponSize offSize = offWeapon->get_weapon_size();
	
	// AD&D 2e Two-Weapon Fighting Rules:
	// 1. Main hand weapon must be MEDIUM or smaller
	// 2. Off-hand weapon must be SMALL or TINY
	// 3. Off-hand weapon must be smaller than or equal to main hand
	
	if (mainSize > WeaponSize::MEDIUM)
	{
		return false; // Main hand weapon too large
	}
	
	if (offSize > WeaponSize::SMALL)
	{
		return false; // Off-hand weapon too large
	}
	
	if (offSize > mainSize)
	{
		return false; // Off-hand larger than main hand
	}
	
	return true;
}

std::unique_ptr<Pickable> Pickable::create(const json& j)
{
	if (!j.contains("type") || !j["type"].is_number()) {
		throw std::runtime_error("Invalid JSON format: Missing or invalid 'type'");
	}

	auto type = static_cast<PickableType>(j["type"].get<int>());
	std::unique_ptr<Pickable> pickable;

	switch (type)
	{
	case PickableType::HEALER:
		pickable = std::make_unique<Healer>(0);
		break;
	case PickableType::LIGHTNING_BOLT:
		pickable = std::make_unique<LightningBolt>(0, 0);
		break;
	case PickableType::CONFUSER:
		pickable = std::make_unique<Confuser>(0, 0);
		break;
	case PickableType::FIREBALL:
		pickable = std::make_unique<Fireball>(0, 0);
		break;
	case PickableType::DAGGER:
		pickable = std::make_unique<Dagger>();
		break;
	case PickableType::LONGSWORD:
		pickable = std::make_unique<LongSword>();
		break;
	case PickableType::SHORTSWORD:
		pickable = std::make_unique<ShortSword>();
		break;
	case PickableType::LONGBOW:
		pickable = std::make_unique<Longbow>();
		break;
	case PickableType::STAFF:
		pickable = std::make_unique<Staff>();
		break;
	case PickableType::GREATSWORD:
		pickable = std::make_unique<Greatsword>();
		break;
	case PickableType::BATTLE_AXE:
		pickable = std::make_unique<BattleAxe>();
		break;
	case PickableType::GREAT_AXE:
		pickable = std::make_unique<GreatAxe>();
		break;
	case PickableType::WAR_HAMMER:
		pickable = std::make_unique<WarHammer>();
		break;
	case PickableType::SHIELD:
		pickable = std::make_unique<Shield>();
		break;
	case PickableType::GOLD:
		pickable = std::make_unique<Gold>(0);
		break;
	case PickableType::FOOD:
		pickable = std::make_unique<Food>(0);
		break;
	case PickableType::CORPSE_FOOD:
		pickable = std::make_unique<CorpseFood>(0);
		break;
	case PickableType::AMULET:
		pickable = std::make_unique<Amulet>();
		break;
	case PickableType::LEATHER_ARMOR:
		pickable = std::make_unique<LeatherArmor>();
		break;
	case PickableType::CHAIN_MAIL:
		pickable = std::make_unique<ChainMail>();
		break;
	case PickableType::PLATE_MAIL:
		pickable = std::make_unique<PlateMail>();
		break;
	default:
		throw std::runtime_error("Unknown PickableType");
	} // end of switch (type)switch (type)

	pickable->load(j);
	return pickable;
}

// Common weapon equip/unequip logic
bool Weapon::use(Item& owner, Creature& wearer)
{
	// Cast to Player to access equipment system
	auto* player = dynamic_cast<Player*>(&wearer);
	if (!player)
	{
		// Fallback to old system for non-player creatures
		return Pickable::use(owner, wearer);
	}

	// If this weapon is already equipped, unequip it
	if (owner.has_state(ActorState::IS_EQUIPPED))
	{
		// Find which slot this weapon is in and unequip it
		for (auto slot : {EquipmentSlot::RIGHT_HAND, EquipmentSlot::LEFT_HAND})
		{
			if (player->get_equipped_item(slot) == &owner)
			{
				player->unequip_item(slot);
				game.message(WHITE_BLACK_PAIR, "You unequip the " + owner.actorData.name + ".", true);
				
				// Remove ranged state if applicable
				if (is_ranged())
				{
					wearer.remove_state(ActorState::IS_RANGED);
				}
				
				return true;
			}
		}
	}

	// Determine equipment slot for new weapon with AD&D 2e dual-wield rules
	EquipmentSlot targetSlot = EquipmentSlot::RIGHT_HAND;
	
	// Check weapon type for slot determination
	if (dynamic_cast<Shield*>(this))
	{
		targetSlot = EquipmentSlot::LEFT_HAND;
	}
	else if (dynamic_cast<Longbow*>(this))
	{
		// Bows go in missile weapon slot
		targetSlot = EquipmentSlot::MISSILE_WEAPON;
	}
	else
	{
		// For regular weapons, check dual-wield possibility
		Item* rightHandWeapon = player->get_equipped_item(EquipmentSlot::RIGHT_HAND);
		Item* leftHandWeapon = player->get_equipped_item(EquipmentSlot::LEFT_HAND);
		
		// Get weapon sizes (need to map from Pickable* to weapon data)
		WeaponSize currentWeaponSize = get_weapon_size();
		
		// If right hand is empty, equip there by default
		if (!rightHandWeapon)
		{
			targetSlot = EquipmentSlot::RIGHT_HAND;
		}
		// If left hand is empty and player wants to dual-wield
		else if (!leftHandWeapon)
		{
			// Check if this weapon can be off-hand
			if (can_be_off_hand(currentWeaponSize))
			{
				// Ask player which hand to use
				game.message(WHITE_BLACK_PAIR, "Equip in: (R)ight hand, (L)eft hand (off-hand), or (C)ancel?", true);
				int choice = getch();
				
				switch (choice)
				{
				case 'r': case 'R':
					// Unequip current right hand weapon first
					if (rightHandWeapon)
					{
						player->unequip_item(EquipmentSlot::RIGHT_HAND);
						game.message(WHITE_BLACK_PAIR, "You unequip your main hand weapon.", true);
					}
					targetSlot = EquipmentSlot::RIGHT_HAND;
					break;
				case 'l': case 'L':
					// Validate dual-wield compatibility
					if (validate_dual_wield(rightHandWeapon, &owner))
					{
						targetSlot = EquipmentSlot::LEFT_HAND;
						game.message(WHITE_BLACK_PAIR, "Dual-wielding setup!", true);
					}
					else
					{
						game.message(WHITE_BLACK_PAIR, "Cannot dual-wield: Off-hand weapon must be smaller than main hand.", true);
						return false;
					}
					break;
				case 'c': case 'C':
					game.message(WHITE_BLACK_PAIR, "Equipping cancelled.", true);
					return false;
				default:
					game.message(WHITE_BLACK_PAIR, "Invalid choice. Equipping in right hand.", true);
					targetSlot = EquipmentSlot::RIGHT_HAND;
					break;
				}
			}
			else
			{
				// Weapon too large for off-hand, replace main hand
				if (rightHandWeapon)
				{
					player->unequip_item(EquipmentSlot::RIGHT_HAND);
					game.message(WHITE_BLACK_PAIR, "You unequip your main hand weapon.", true);
				}
				targetSlot = EquipmentSlot::RIGHT_HAND;
			}
		}
		// Both hands occupied
		else
		{
			game.message(WHITE_BLACK_PAIR, "Both hands occupied. Which to replace: (R)ight hand or (L)eft hand?", true);
			int choice = getch();
			
			switch (choice)
			{
			case 'r': case 'R':
				player->unequip_item(EquipmentSlot::RIGHT_HAND);
				targetSlot = EquipmentSlot::RIGHT_HAND;
				break;
			case 'l': case 'L':
				// Validate if this weapon can be off-hand
				if (validate_dual_wield(rightHandWeapon, &owner))
				{
					player->unequip_item(EquipmentSlot::LEFT_HAND);
					targetSlot = EquipmentSlot::LEFT_HAND;
				}
				else
				{
					game.message(WHITE_BLACK_PAIR, "Cannot use as off-hand weapon.", true);
					return false;
				}
				break;
			default:
				game.message(WHITE_BLACK_PAIR, "Invalid choice.", true);
				return false;
			}
		}
	}

	// Check if we can equip in the target slot
	if (!player->can_equip(owner, targetSlot))
	{
		game.message(WHITE_BLACK_PAIR, "Cannot equip " + owner.actorData.name + " - slot is occupied!", true);
		return false; // Don't consume turn if can't equip
	}

	// Remove the item from inventory and equip it
	auto compareItems = [&owner](const std::unique_ptr<Item>& item) { return item.get() == &owner; };
	auto it = std::find_if(wearer.container->get_inventory_mutable().begin(), wearer.container->get_inventory_mutable().end(), compareItems);
	
	if (it != wearer.container->get_inventory_mutable().end())
	{
		// Move item from inventory to equipment
		std::unique_ptr<Item> item = std::move(*it);
		wearer.container->get_inventory_mutable().erase(it);
		
		// Equip the item in the new equipment system
		if (player->equip_item(std::move(item), targetSlot))
		{
			// Update combat stats
			if (targetSlot == EquipmentSlot::RIGHT_HAND)
			{
				wearer.attacker->roll = this->roll;
				
				// Apply ranged state if applicable
				if (is_ranged())
				{
					wearer.add_state(ActorState::IS_RANGED);
				}
			}
			
			game.message(WHITE_BLACK_PAIR, "You equip the " + owner.actorData.name + ".", true);
			return true;
		}
		else
		{
			// If equipping failed, return item to inventory
			wearer.container->add(std::move(item));
		}
	}

	return false;
}

// Implement the Dagger class
bool Dagger::is_ranged() const
{
	return false;
}

void Dagger::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::DAGGER); // Save the type
	j["roll"] = roll; // Save the roll string
}

void Dagger::load(const json& j)
{
	if (j.contains("roll") && j["roll"].is_string()) {
		roll = j["roll"].get<std::string>(); // Load the roll string
	}
	else {
		throw std::runtime_error("Invalid JSON format for Dagger");
	}
}

// Implement the LongSword class
bool LongSword::is_ranged() const
{
	return false;
}

void LongSword::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::LONGSWORD);
	j["roll"] = roll;
}

void LongSword::load(const json& j)
{
	if (j.contains("roll") && j["roll"].is_string()) {
		roll = j["roll"].get<std::string>();
	}
	else {
		throw std::runtime_error("Invalid JSON format for LongSword");
	}
}

// Implement the ShortSword class
bool ShortSword::is_ranged() const
{
	return false;
}

void ShortSword::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::SHORTSWORD);
	j["roll"] = roll;
}

void ShortSword::load(const json& j)
{
	if (j.contains("roll") && j["roll"].is_string()) {
		roll = j["roll"].get<std::string>();
	}
	else {
		throw std::runtime_error("Invalid JSON format for ShortSword");
	}
}

// Implement the Longbow class
bool Longbow::is_ranged() const
{
	return true;
}

void Longbow::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::LONGBOW);
	j["roll"] = roll;
}

void Longbow::load(const json& j)
{
	if (j.contains("roll") && j["roll"].is_string()) {
		roll = j["roll"].get<std::string>();
	}
	else {
		throw std::runtime_error("Invalid JSON format for Longbow");
	}
}

// Implement the Staff class
bool Staff::is_ranged() const
{
	return false;
}

void Staff::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::STAFF);
	j["roll"] = roll;
}

void Staff::load(const json& j)
{
if (j.contains("roll") && j["roll"].is_string()) {
roll = j["roll"].get<std::string>();
}
else {
throw std::runtime_error("Invalid JSON format for Staff");
}
}

	// Implement the Greatsword class
	bool Greatsword::is_ranged() const
	{
		return false;
	}

	void Greatsword::save(json& j)
	{
		j["type"] = static_cast<int>(PickableType::GREATSWORD);
		j["roll"] = roll;
	}

	void Greatsword::load(const json& j)
	{
		if (j.contains("roll") && j["roll"].is_string()) {
			roll = j["roll"].get<std::string>();
		}
		else {
			throw std::runtime_error("Invalid JSON format for Greatsword");
		}
	}

	// Implement the BattleAxe class
	bool BattleAxe::is_ranged() const
	{
		return false;
	}

	void BattleAxe::save(json& j)
	{
		j["type"] = static_cast<int>(PickableType::BATTLE_AXE);
		j["roll"] = roll;
	}

	void BattleAxe::load(const json& j)
	{
		if (j.contains("roll") && j["roll"].is_string()) {
			roll = j["roll"].get<std::string>();
		}
		else {
			throw std::runtime_error("Invalid JSON format for BattleAxe");
		}
	}

	// Implement the GreatAxe class
	bool GreatAxe::is_ranged() const
	{
		return false;
	}

	void GreatAxe::save(json& j)
	{
		j["type"] = static_cast<int>(PickableType::GREAT_AXE);
		j["roll"] = roll;
	}

	void GreatAxe::load(const json& j)
	{
		if (j.contains("roll") && j["roll"].is_string()) {
			roll = j["roll"].get<std::string>();
		}
		else {
			throw std::runtime_error("Invalid JSON format for GreatAxe");
		}
	}

	// Implement the WarHammer class
	bool WarHammer::is_ranged() const
	{
		return false;
	}

	void WarHammer::save(json& j)
	{
		j["type"] = static_cast<int>(PickableType::WAR_HAMMER);
		j["roll"] = roll;
	}

	void WarHammer::load(const json& j)
	{
		if (j.contains("roll") && j["roll"].is_string()) {
			roll = j["roll"].get<std::string>();
		}
		else {
			throw std::runtime_error("Invalid JSON format for WarHammer");
		}
	}

	// Implement the Shield class
	bool Shield::is_ranged() const
	{
		return false;
	}

	void Shield::save(json& j)
	{
		j["type"] = static_cast<int>(PickableType::SHIELD);
		j["roll"] = roll;
	}

	void Shield::load(const json& j)
	{
		if (j.contains("roll") && j["roll"].is_string()) {
			roll = j["roll"].get<std::string>();
		}
		else {
			throw std::runtime_error("Invalid JSON format for Shield");
		}
	}