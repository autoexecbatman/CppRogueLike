#include "Armor.h"
#include "Game.h"
#include "Colors/Colors.h"

// Common armor equip/unequip logic
bool Armor::use(Item& owner, Creature& wearer)
{
    // Check if this armor is being directly unequipped
    if (owner.has_state(ActorState::IS_EQUIPPED))
    {
        // Unequip the armor
        owner.remove_state(ActorState::IS_EQUIPPED);
        game.message(WHITE_PAIR, "You remove the " + owner.actorData.name + ".", true);

        // Recalculate AC
        wearer.destructible->update_armor_class(wearer);
        game.message(WHITE_PAIR, "Your armor class is now " + std::to_string(wearer.destructible->armorClass) + ".", true);

        return false; // Don't consume the armor
    }

    // Check if any armor is already equipped
    Item* currentArmor = nullptr;
    for (const auto& item : wearer.container->inv)
    {
        if (item && item->has_state(ActorState::IS_EQUIPPED) &&
            item.get() != &owner &&
            dynamic_cast<Armor*>(item->pickable.get()))
        {
            currentArmor = item.get();
            break;
        }
    }

    // If we're trying to equip and another armor is already equipped, unequip it first
    if (currentArmor)
    {
        game.message(WHITE_PAIR, "You remove your " + currentArmor->actorData.name + ".", true);
        currentArmor->remove_state(ActorState::IS_EQUIPPED);
    }

    // Equip the armor
    owner.add_state(ActorState::IS_EQUIPPED);
    game.message(WHITE_PAIR, "You put on the " + owner.actorData.name + ".", true);

    // Recalculate AC
    wearer.destructible->update_armor_class(wearer);
    game.message(WHITE_PAIR, "Your armor class is now " + std::to_string(wearer.destructible->armorClass) + ".", true);

    return false; // Don't consume the armor
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
        armorClass = 2; // Default value
    }
}