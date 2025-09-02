#include <string>

#include "Weapons.h"

int Weapons::get_min_hit_bonus() const
{
    return hitBonusRange.empty() ? 0 : hitBonusRange.front();
}

int Weapons::get_max_hit_bonus() const
{
    return hitBonusRange.empty() ? 0 : hitBonusRange.back();
}

int Weapons::get_min_damage_bonus() const
{
    return damageBonusRange.empty() ? 0 : damageBonusRange.front();
}

int Weapons::get_max_damage_bonus() const
{
    return damageBonusRange.empty() ? 0 : damageBonusRange.back();
}

std::string Weapons::get_display_name() const
{
    if (enhancementLevel > 0) {
        return name + " +" + std::to_string(enhancementLevel);
    }
    return name;
}

void Weapons::set_enhancement_level(int level)
{
    enhancementLevel = level;
}

void Weapons::enhance_weapon(int levels)
{
    enhancementLevel += levels;
}

std::string Weapons::get_damage_roll(bool twoHanded) const noexcept
{
    if (twoHanded && !damageRollTwoHanded.empty()) {
        return damageRollTwoHanded;
    }
    return damageRoll;
}

bool Weapons::is_compatible_off_hand(const Weapons& mainHandWeapon) const noexcept
{
    return can_be_off_hand() && mainHandWeapon.weaponSize >= weaponSize;
}

bool Weapons::can_dual_wield(const Weapons& mainHand, const Weapons& offHand) noexcept
{
    return mainHand.can_be_main_hand() && offHand.is_compatible_off_hand(mainHand);
}
