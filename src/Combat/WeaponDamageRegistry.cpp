// WeaponDamageRegistry.cpp - Implementation using ItemId system
#include "WeaponDamageRegistry.h"
#include "../Systems/ItemEnhancements/ItemEnhancements.h"

// Static member definition
const std::unordered_map<ItemId, DamageInfo> WeaponDamageRegistry::weapon_damage_map =
    WeaponDamageRegistry::create_weapon_damage_map();

std::unordered_map<ItemId, DamageInfo> WeaponDamageRegistry::create_weapon_damage_map()
{
    return {
        // Melee Weapons - AD&D 2e damage values
        {ItemId::DAGGER,      DamageValues::Dagger()},       // 1-4 damage
        {ItemId::SHORT_SWORD, DamageValues::ShortSword()},   // 1-6 damage
        {ItemId::LONG_SWORD,  DamageValues::LongSword()},    // 1-8 damage
        {ItemId::STAFF,       DamageValues::Staff()},        // 1-6 damage
        {ItemId::BATTLE_AXE,  DamageValues::BattleAxe()},    // 1-8 damage
        {ItemId::GREAT_AXE,   {1, 12, "1d12"}},             // 1-12 damage
        {ItemId::WAR_HAMMER,  DamageValues::WarHammer()},    // 2-5 damage (1d4+1)
        {ItemId::MACE,        {2, 7, "1d6+1"}},             // 2-7 damage (1d6+1)
        {ItemId::GREAT_SWORD, DamageValues::GreatSword()},   // 1-10 damage

        // Ranged Weapons
        {ItemId::LONG_BOW,    DamageValues::LongBow()},      // 1-6 damage
    };
}

DamageInfo WeaponDamageRegistry::get_damage_info(ItemId weaponId) noexcept
{
    if (weapon_damage_map.contains(weaponId))
    {
        return weapon_damage_map.at(weaponId);
    }

    // Fallback to unarmed damage for unregistered weapons
    return get_unarmed_damage_info();
}

DamageInfo WeaponDamageRegistry::get_enhanced_damage_info(ItemId weaponId, const ItemEnhancement* enhancement) noexcept
{
    DamageInfo baseDamage = get_damage_info(weaponId);

    if (enhancement && enhancement->damage_bonus != 0)
    {
        return baseDamage.with_enhancement(enhancement->damage_bonus, 0);
    }

    return baseDamage;
}

std::string WeaponDamageRegistry::get_damage_roll(ItemId weaponId) noexcept
{
    auto damageInfo = get_damage_info(weaponId);
    return damageInfo.displayRoll;
}

bool WeaponDamageRegistry::is_registered(ItemId weaponId) noexcept
{
    return weapon_damage_map.contains(weaponId);
}
