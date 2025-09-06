#include "Armor.h"
#include "../Game.h"
#include "../Colors/Colors.h"
#include "../ActorTypes/Player.h"

// Clean armor equip/unequip logic - delegates to Equipment System
bool Armor::use(Item& item, Creature& wearer)
{
	// Use the item's own name and type directly
	std::string armorName = item.actorData.name;
	
	// For players, delegate to Equipment System using unique IDs
	if (wearer.uniqueId == game.player->uniqueId)
	{
		Player* player = static_cast<Player*>(&wearer);
		
		// Clean delegation using unique ID
		bool wasEquipped = player->is_item_equipped(item.uniqueId);
		bool success = player->toggle_armor(item.uniqueId);
		
		if (success)
		{
			if (wasEquipped)
			{
				game.message(WHITE_BLACK_PAIR, "You remove the " + armorName + ".", true);
			}
			else
			{
				game.message(WHITE_BLACK_PAIR, "You put on the " + armorName + ".", true);
			}
		}
		
		return success;
	}
	
	// For NPCs, use simple stat modification
	apply_stat_effects(wearer, item);
	return true;
}

// Pure stat effects for NPCs - no inventory management
void Armor::apply_stat_effects(Creature& creature, Item& owner)
{
	if (owner.has_state(ActorState::IS_EQUIPPED))
	{
		// Remove armor effects
		owner.remove_state(ActorState::IS_EQUIPPED);
		creature.destructible->update_armor_class(creature);
	}
	else
	{
		// Apply armor effects
		owner.add_state(ActorState::IS_EQUIPPED);
		creature.destructible->update_armor_class(creature);
	}
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