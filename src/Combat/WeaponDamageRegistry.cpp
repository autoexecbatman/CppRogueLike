// WeaponDamageRegistry.cpp - Implementation using ItemClass system
#include "WeaponDamageRegistry.h"

// Static member definition
const std::unordered_map<ItemClass, DamageInfo> WeaponDamageRegistry::weapon_damage_map = 
    WeaponDamageRegistry::create_weapon_damage_map();

std::unordered_map<ItemClass, DamageInfo> WeaponDamageRegistry::create_weapon_damage_map()
{
    return {
        // Melee Weapons - AD&D 2e damage values
        {ItemClass::DAGGER,     DamageValues::Dagger()},      // 1-4 damage
        {ItemClass::SHORT_SWORD, DamageValues::ShortSword()},  // 1-6 damage
        {ItemClass::LONG_SWORD,  DamageValues::LongSword()},   // 1-8 damage
        {ItemClass::STAFF,      DamageValues::Staff()},       // 1-6 damage
        {ItemClass::BATTLE_AXE, DamageValues::BattleAxe()},   // 1-8 damage
        {ItemClass::GREAT_AXE,  {1, 12, "1d12"}},            // 1-12 damage
        {ItemClass::WAR_HAMMER, DamageValues::WarHammer()},   // 2-5 damage (1d4+1)
        {ItemClass::GREAT_SWORD, DamageValues::GreatSword()},  // 1-10 damage
        
        // Ranged Weapons
        {ItemClass::LONG_BOW,    DamageValues::LongBow()},     // 1-6 damage
    };
}

DamageInfo WeaponDamageRegistry::get_damage_info(ItemClass weaponClass) noexcept
{
    auto it = weapon_damage_map.find(weaponClass);
    if (it != weapon_damage_map.end())
    {
        return it->second;
    }
    
    // Fallback to unarmed damage for unregistered weapons
    return get_unarmed_damage_info();
}

std::string WeaponDamageRegistry::get_damage_roll(ItemClass weaponClass) noexcept
{
    auto damageInfo = get_damage_info(weaponClass);
    return damageInfo.displayRoll;
}

bool WeaponDamageRegistry::is_registered(ItemClass weaponClass) noexcept
{
    return weapon_damage_map.find(weaponClass) != weapon_damage_map.end();
}
