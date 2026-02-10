#pragma once

#include <string>
#include <unordered_map>

#include "DamageInfo.h"
#include "../Items/ItemClassification.h"

// Forward declarations
struct ItemEnhancement;

class WeaponDamageRegistry
{
public:
    // Get damage info for weapon ID (main interface)
    static DamageInfo get_damage_info(ItemId weaponId) noexcept;

    // Get enhanced damage info with weapon modifiers
    static DamageInfo get_enhanced_damage_info(ItemId weaponId, const ItemEnhancement* enhancement) noexcept;

    // Legacy support - get damage roll string for display
    static std::string get_damage_roll(ItemId weaponId) noexcept;

    // Check if weapon ID is registered
    static bool is_registered(ItemId weaponId) noexcept;

    // Get default unarmed damage
    static DamageInfo get_unarmed_damage_info() noexcept { return DamageValues::Unarmed(); }
    static std::string get_unarmed_damage() noexcept { return "1d2"; }

private:
    // Centralized weapon damage mapping using ItemId
    static const std::unordered_map<ItemId, DamageInfo> weapon_damage_map;

    // Initialize static data
    static std::unordered_map<ItemId, DamageInfo> create_weapon_damage_map();
};
