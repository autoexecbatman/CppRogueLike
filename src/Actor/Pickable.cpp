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
		std::erase_if(wearer.container->inv, compareItems);

		return true;
	}

	return false;
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
		for (auto slot : {EquipmentSlot::MAIN_HAND, EquipmentSlot::OFF_HAND})
		{
			if (player->getEquippedItem(slot) == &owner)
			{
				player->unequipItem(slot);
				game.message(WHITE_BLACK_PAIR, "You unequip the " + owner.actorData.name + ".", true);
				
				// Remove ranged state if applicable
				if (isRanged())
				{
					wearer.remove_state(ActorState::IS_RANGED);
				}
				
				return true;
			}
		}
	}

	// Determine equipment slot and hand usage for new weapon
	EquipmentSlot targetSlot = EquipmentSlot::MAIN_HAND;
	bool useTwoHanded = false;

	// Check weapon type for slot determination
	if (dynamic_cast<Shield*>(this))
	{
		targetSlot = EquipmentSlot::OFF_HAND;
	}
	else if (dynamic_cast<Greatsword*>(this) || dynamic_cast<GreatAxe*>(this) || 
			 dynamic_cast<Longbow*>(this))
	{
		// Pure two-handed weapons
		targetSlot = EquipmentSlot::MAIN_HAND;
		useTwoHanded = true;
	}
	else if (dynamic_cast<LongSword*>(this) || dynamic_cast<BattleAxe*>(this) || 
			 dynamic_cast<WarHammer*>(this) || dynamic_cast<Staff*>(this))
	{
		// Versatile weapons - check if user wants two-handed
		targetSlot = EquipmentSlot::MAIN_HAND;
		
		// If off-hand is free, ask if they want to use two-handed
		if (!player->isSlotOccupied(EquipmentSlot::OFF_HAND))
		{
			// For now, default to one-handed unless off-hand is empty
			useTwoHanded = false;
			game.message(WHITE_BLACK_PAIR, "Using " + owner.actorData.name + " one-handed. Use 'T' to toggle two-handed grip.", true);
		}
		else
		{
			useTwoHanded = false;
		}
	}

	// Check if we can equip in the target slot
	if (!player->canEquip(owner, targetSlot, useTwoHanded))
	{
		if (useTwoHanded)
		{
			game.message(WHITE_BLACK_PAIR, "Cannot equip " + owner.actorData.name + " - your off-hand is occupied!", true);
		}
		else if (targetSlot == EquipmentSlot::OFF_HAND)
		{
			game.message(WHITE_BLACK_PAIR, "Cannot equip shield - you're using a two-handed weapon!", true);
		}
		else
		{
			game.message(WHITE_BLACK_PAIR, "Cannot equip " + owner.actorData.name + " - slot is occupied!", true);
		}
		return false; // Don't consume turn if can't equip
	}

	// Remove the item from inventory and equip it
	auto compareItems = [&owner](const std::unique_ptr<Item>& item) { return item.get() == &owner; };
	auto it = std::find_if(wearer.container->inv.begin(), wearer.container->inv.end(), compareItems);
	
	if (it != wearer.container->inv.end())
	{
		// Move item from inventory to equipment
		std::unique_ptr<Item> item = std::move(*it);
		wearer.container->inv.erase(it);
		
		// Equip the item in the new equipment system
		if (player->equipItem(std::move(item), targetSlot, useTwoHanded))
		{
			// Update combat stats
			if (targetSlot == EquipmentSlot::MAIN_HAND)
			{
				wearer.attacker->roll = this->roll;
				
				// Apply ranged state if applicable
				if (isRanged())
				{
					wearer.add_state(ActorState::IS_RANGED);
				}
			}
			
			game.message(WHITE_BLACK_PAIR, "You equip the " + owner.actorData.name + 
						 (useTwoHanded ? " (two-handed)." : "."), true);
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
bool Dagger::isRanged() const
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
bool LongSword::isRanged() const
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
bool ShortSword::isRanged() const
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
bool Longbow::isRanged() const
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
bool Staff::isRanged() const
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
	bool Greatsword::isRanged() const
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
	bool BattleAxe::isRanged() const
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
	bool GreatAxe::isRanged() const
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
	bool WarHammer::isRanged() const
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
	bool Shield::isRanged() const
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