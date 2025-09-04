// PickableTypeRegistry.h - Type registry for eliminating dynamic_cast
#ifndef PICKABLE_TYPE_REGISTRY_H
#define PICKABLE_TYPE_REGISTRY_H

#pragma once

#include "../Actor/Pickable.h"
#include <unordered_map>
#include <string>

namespace PickableTypeRegistry
{
    // Enum for all pickable types - eliminates dynamic_cast needs
    enum class Type : int
    {
        UNKNOWN = 0,
        HEALER,
        LIGHTNING_BOLT,
        CONFUSER,
        FIREBALL,
        LONGSWORD,
        DAGGER,
        SHORTSWORD,
        LONGBOW,
        STAFF,
        GREATSWORD,
        BATTLE_AXE,
        GREAT_AXE,
        WAR_HAMMER,
        SHIELD,
        GOLD,
        FOOD,
        CORPSE_FOOD,
        AMULET,
        LEATHER_ARMOR,
        CHAIN_MAIL,
        PLATE_MAIL
    };

    // Category classification
    enum class Category : int
    {
        UNKNOWN = 0,
        WEAPON,
        ARMOR, 
        CONSUMABLE,
        TREASURE,
        TOOL
    };

    // Get type from pickable instance (replaces dynamic_cast)
    Type get_type(const Pickable* pickable);
    
    // Get category from type
    Category get_category(Type type);
    
    // Get display name for type
    std::string get_display_name(Type type);
    
    // Helper to get item type from weapon (avoids dynamic_cast in weapon validation)
    Type get_weapon_type(const Weapon* weapon);
    
    // Get weapon size from type (replaces dynamic_cast weapon size detection)
    WeaponSize get_weapon_size(Type type);
    
    // Type checking functions (replacements for dynamic_cast)
    inline bool is_weapon(Type type) { return get_category(type) == Category::WEAPON; }
    inline bool is_armor(Type type) { return get_category(type) == Category::ARMOR; }
    inline bool is_gold(Type type) { return type == Type::GOLD; }
    inline bool is_ranged_weapon(Type type) { return type == Type::LONGBOW; }
    
    // Get type from item safely
    Type get_item_type(const Item& item);
}

#endif // PICKABLE_TYPE_REGISTRY_H
