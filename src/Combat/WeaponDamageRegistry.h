// WeaponDamageRegistry.h - Centralized weapon damage mapping with ItemClass
#ifndef WEAPON_DAMAGE_REGISTRY_H
#define WEAPON_DAMAGE_REGISTRY_H

#pragma once

#include <unordered_map>
#include <string>
#include "DamageInfo.h"
#include "../Items/ItemClassification.h"

// Forward declarations
struct ItemEnhancement;

class WeaponDamageRegistry
{
public:
    // Get damage info for weapon class (main interface)
    static DamageInfo get_damage_info(ItemClass weaponClass) noexcept;

    // Get enhanced damage info with weapon modifiers
    static DamageInfo get_enhanced_damage_info(ItemClass weaponClass, const ItemEnhancement* enhancement) noexcept;

    // Legacy support - get damage roll string for display
    static std::string get_damage_roll(ItemClass weaponClass) noexcept;

    // Check if weapon class is registered
    static bool is_registered(ItemClass weaponClass) noexcept;
    
    // Get default unarmed damage
    static DamageInfo get_unarmed_damage_info() noexcept { return DamageValues::Unarmed(); }
    static std::string get_unarmed_damage() noexcept { return "1d2"; }

private:
    // Centralized weapon damage mapping using ItemClass
    static const std::unordered_map<ItemClass, DamageInfo> weapon_damage_map;
    
    // Initialize static data
    static std::unordered_map<ItemClass, DamageInfo> create_weapon_damage_map();
};

#endif // WEAPON_DAMAGE_REGISTRY_H