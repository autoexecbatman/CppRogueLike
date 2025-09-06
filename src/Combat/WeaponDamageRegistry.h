// WeaponDamageRegistry.h - Centralized weapon damage mapping with DamageInfo
#ifndef WEAPON_DAMAGE_REGISTRY_H
#define WEAPON_DAMAGE_REGISTRY_H

#pragma once

#include <unordered_map>
#include <string>
#include "DamageInfo.h"
#include "../Utils/PickableTypeRegistry.h"

class WeaponDamageRegistry
{
public:
    // Get damage info for weapon type (main interface)
    static DamageInfo get_damage_info(PickableTypeRegistry::Type weaponType) noexcept;
    
    // Legacy support - get damage roll string for display
    static std::string get_damage_roll(PickableTypeRegistry::Type weaponType) noexcept;
    
    // Check if weapon type is registered
    static bool is_registered(PickableTypeRegistry::Type weaponType) noexcept;
    
    // Get default unarmed damage
    static DamageInfo get_unarmed_damage_info() noexcept { return DamageValues::Unarmed(); }
    static std::string get_unarmed_damage() noexcept { return "1d2"; }

private:
    // Centralized weapon damage mapping
    static const std::unordered_map<PickableTypeRegistry::Type, DamageInfo> weapon_damage_map;
    
    // Initialize static data
    static std::unordered_map<PickableTypeRegistry::Type, DamageInfo> create_weapon_damage_map();
};

#endif // WEAPON_DAMAGE_REGISTRY_H