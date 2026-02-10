#include "Armor.h"
#include "../Colors/Colors.h"
#include "../ActorTypes/Player.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"

// Clean armor equip/unequip logic - delegates to Equipment System
bool Armor::use(Item& item, Creature& wearer, GameContext& ctx)
{
	// Use the item's own name and type directly
	std::string armorName = item.actorData.name;
	
	// For players, delegate to Equipment System using unique IDs
	if (wearer.uniqueId == ctx.player->uniqueId)
	{
		Player* player = static_cast<Player*>(&wearer);
		
		// Clean delegation using unique ID
		bool wasEquipped = player->is_item_equipped(item.uniqueId);
		bool success = player->toggle_armor(item.uniqueId, ctx);
		
		if (success)
		{
			if (wasEquipped)
			{
				ctx.message_system->message(WHITE_BLACK_PAIR, "You remove the " + armorName + ".", true);
			}
			else
			{
				ctx.message_system->message(WHITE_BLACK_PAIR, "You put on the " + armorName + ".", true);
			}
		}
		
		return success;
	}
	
	// For NPCs, use simple stat modification
	apply_stat_effects(wearer, item, ctx);
	return true;
}

// Pure stat effects for NPCs - no inventory management
void Armor::apply_stat_effects(Creature& creature, Item& owner, GameContext& ctx)
{
	
	if (owner.has_state(ActorState::IS_EQUIPPED))
	{
		// Remove armor effects
		owner.remove_state(ActorState::IS_EQUIPPED);
		creature.destructible->update_armor_class(creature, ctx);
	}
	else
	{
		// Apply armor effects
		owner.add_state(ActorState::IS_EQUIPPED);
		creature.destructible->update_armor_class(creature, ctx);
	}
}

// LeatherArmor implementation
LeatherArmor::LeatherArmor()
{
    armorClass = -2; // Leather armor provides AC 2 bonus
}

void LeatherArmor::save(json& j)
{
    j["type"] = static_cast<int>(PickableType::ARMOR);
    j["armorClass"] = armorClass;
}

void LeatherArmor::load(const json& j)
{
    armorClass = j.at("armorClass").get<int>();
}

// ChainMail implementation
ChainMail::ChainMail()
{
    armorClass = -4; // Chain mail provides AC 4 bonus (better than leather)
}

void ChainMail::save(json& j)
{
    j["type"] = static_cast<int>(PickableType::ARMOR);
    j["armorClass"] = armorClass;
}

void ChainMail::load(const json& j)
{
    armorClass = j.at("armorClass").get<int>();
}

// PlateMail implementation
PlateMail::PlateMail()
{
    armorClass = -6; // Plate mail provides AC 6 bonus (better than chain mail)
}

void PlateMail::save(json& j)
{
    j["type"] = static_cast<int>(PickableType::ARMOR);
    j["armorClass"] = armorClass;
}

void PlateMail::load(const json& j)
{
    armorClass = j.at("armorClass").get<int>();
}