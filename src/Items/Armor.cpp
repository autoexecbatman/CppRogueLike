#include "Armor.h"
#include "../Game.h"
#include "../Colors/Colors.h"
#include "../ActorTypes/Player.h"

// Common armor equip/unequip logic
bool Armor::use(Item& owner, Creature& wearer)
{
	// Cast to Player to use the equipment system
	auto* player = dynamic_cast<Player*>(&wearer);
	if (!player)
	{
		// Fallback for non-player creatures (use old system)
		if (owner.has_state(ActorState::IS_EQUIPPED))
		{
			owner.remove_state(ActorState::IS_EQUIPPED);
			game.message(WHITE_BLACK_PAIR, "You remove the " + owner.actorData.name + ".", true);
			wearer.destructible->update_armor_class(wearer);
			game.message(WHITE_BLACK_PAIR, "Your armor class is now " + std::to_string(wearer.destructible->armorClass) + ".", true);
			return true;
		}
		
		// Old equip logic for non-players
		owner.add_state(ActorState::IS_EQUIPPED);
		game.message(WHITE_BLACK_PAIR, "You put on the " + owner.actorData.name + ".", true);
		wearer.destructible->update_armor_class(wearer);
		game.message(WHITE_BLACK_PAIR, "Your armor class is now " + std::to_string(wearer.destructible->armorClass) + ".", true);
		return true;
	}
	
	// For players, use the new equipment system
	// Check if this armor is already equipped
	Item* currentArmor = player->get_equipped_item(EquipmentSlot::BODY);
	if (currentArmor == &owner)
	{
		// Unequip this armor - the unequipItem method handles AC update and messaging
		player->unequip_item(EquipmentSlot::BODY);
		game.message(WHITE_BLACK_PAIR, "You remove the " + owner.actorData.name + ".", true);
		return true;
	}
	
	// Try to equip the armor
	// First, find this item in the inventory
	std::unique_ptr<Item> itemToEquip = nullptr;
	for (auto it = wearer.container->get_inventory_mutable().begin(); it != wearer.container->get_inventory_mutable().end(); ++it)
	{
		if (it->get() == &owner)
		{
			itemToEquip = std::move(*it);
			wearer.container->get_inventory_mutable().erase(it);
			break;
		}
	}
	
	if (itemToEquip)
	{
		// Equip the armor using the equipment system - equipItem handles AC update and messaging
		if (player->equip_item(std::move(itemToEquip), EquipmentSlot::BODY))
		{
			game.message(WHITE_BLACK_PAIR, "You put on the " + owner.actorData.name + ".", true);
			return true;
		}
		else
		{
			// Failed to equip, return item to inventory
			wearer.container->get_inventory_mutable().push_back(std::move(itemToEquip));
			game.message(WHITE_BLACK_PAIR, "You can't equip the " + owner.actorData.name + ".", true);
			return false;
		}
	}
	
	game.message(WHITE_BLACK_PAIR, "Failed to equip armor.", true);
	return false;
}

// LeatherArmor implementation
LeatherArmor::LeatherArmor()
{
    armorClass = -2; // Leather armor provides AC 2 bonus
}

void LeatherArmor::save(json& j)
{
    j["type"] = static_cast<int>(PickableType::LEATHER_ARMOR);
    j["armorClass"] = armorClass;
}

void LeatherArmor::load(const json& j)
{
    if (j.contains("armorClass") && j["armorClass"].is_number())
    {
        armorClass = j["armorClass"].get<int>();
    }
    else
    {
        armorClass = -2; // Default value
    }
}

// ChainMail implementation
ChainMail::ChainMail()
{
    armorClass = -4; // Chain mail provides AC 4 bonus (better than leather)
}

void ChainMail::save(json& j)
{
    j["type"] = static_cast<int>(PickableType::CHAIN_MAIL);
    j["armorClass"] = armorClass;
}

void ChainMail::load(const json& j)
{
    if (j.contains("armorClass") && j["armorClass"].is_number())
    {
        armorClass = j["armorClass"].get<int>();
    }
    else
    {
        armorClass = -4; // Default value
    }
}

// PlateMail implementation
PlateMail::PlateMail()
{
    armorClass = -6; // Plate mail provides AC 6 bonus (better than chain mail)
}

void PlateMail::save(json& j)
{
    j["type"] = static_cast<int>(PickableType::PLATE_MAIL);
    j["armorClass"] = armorClass;
}

void PlateMail::load(const json& j)
{
    if (j.contains("armorClass") && j["armorClass"].is_number())
    {
        armorClass = j["armorClass"].get<int>();
    }
    else
    {
        armorClass = -6; // Default value
    }
}