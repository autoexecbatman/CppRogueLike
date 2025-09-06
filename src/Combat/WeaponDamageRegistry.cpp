// WeaponDamageRegistry.cpp - Implementation of robust damage value system
#include "WeaponDamageRegistry.h"

// Static member definition
const std::unordered_map<PickableTypeRegistry::Type, DamageInfo> WeaponDamageRegistry::weapon_damage_map = 
    WeaponDamageRegistry::create_weapon_damage_map();

std::unordered_map<PickableTypeRegistry::Type, DamageInfo> WeaponDamageRegistry::create_weapon_damage_map()
{
    return {
        // Melee Weapons - AD&D 2e damage values
        {PickableTypeRegistry::Type::DAGGER,     DamageValues::Dagger()},      // 1-4 damage
        {PickableTypeRegistry::Type::SHORTSWORD, DamageValues::ShortSword()},  // 1-6 damage
        {PickableTypeRegistry::Type::LONGSWORD,  DamageValues::LongSword()},   // 1-8 damage
        {PickableTypeRegistry::Type::STAFF,      DamageValues::Staff()},       // 1-6 damage
        {PickableTypeRegistry::Type::BATTLE_AXE, DamageValues::BattleAxe()},   // 1-8 damage
        {PickableTypeRegistry::Type::GREAT_AXE,  {1, 12, "1d12"}},            // 1-12 damage
        {PickableTypeRegistry::Type::WAR_HAMMER, DamageValues::WarHammer()},   // 2-5 damage (1d4+1)
        {PickableTypeRegistry::Type::GREATSWORD, DamageValues::GreatSword()},  // 1-10 damage
        
        // Ranged Weapons
        {PickableTypeRegistry::Type::LONGBOW,    DamageValues::LongBow()},     // 1-6 damage
    };
}

DamageInfo WeaponDamageRegistry::get_damage_info(PickableTypeRegistry::Type weaponType) noexcept
{
    auto it = weapon_damage_map.find(weaponType);
    if (it != weapon_damage_map.end())
    {
        return it->second;
    }
    
    // Fallback to unarmed damage for unregistered weapons
    return get_unarmed_damage_info();
}

std::string WeaponDamageRegistry::get_damage_roll(PickableTypeRegistry::Type weaponType) noexcept
{
    auto damageInfo = get_damage_info(weaponType);
    return damageInfo.displayRoll;
}

bool WeaponDamageRegistry::is_registered(PickableTypeRegistry::Type weaponType) noexcept
{
    return weapon_damage_map.find(weaponType) != weapon_damage_map.end();
}
