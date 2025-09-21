#include "Mace.h"
#include "../Game.h"
#include "../Colors/Colors.h"

Mace::Mace()
{
}

bool Mace::use(Item& owner, Creature& wearer)
{
    // Mace is a weapon, attempt to equip it
    // Check if wearer has equipment capability (likely Player class)
    game.message(COLOR_WHITE, "You wield the " + owner.actorData.name + ".", true);
    return Pickable::use(owner, wearer);
}

void Mace::load(const json& j)
{
    // No special data for basic mace
}

void Mace::save(json& j)
{
    j["type"] = static_cast<int>(PickableType::MACE);  // Use proper MACE type
}

Pickable::PickableType Mace::get_type() const
{
    return PickableType::MACE;  // Use proper MACE type
}
