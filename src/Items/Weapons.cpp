#include <string>

#include "Weapons.h"

// Deprecated enhancement and bonus methods removed

std::string Weapons::get_damage_roll(bool twoHanded) const noexcept
{
    if (twoHanded && !damageRollTwoHanded.empty()) {
        return damageRollTwoHanded;
    }
    return damageRoll;
}

// Deprecated dual-wield methods removed
