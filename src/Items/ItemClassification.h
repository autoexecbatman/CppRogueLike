// ItemClassification.h - Proper item classification system separate from pickable components
#ifndef ITEM_CLASSIFICATION_H
#define ITEM_CLASSIFICATION_H

#pragma once

#include <string>
#include "Weapons.h" // For WeaponSize enum

enum class ItemClass
{
    // Unknown/Default
    UNKNOWN = 0,
    
    // Weapons - Melee
    DAGGER,
    SHORT_SWORD,
    LONG_SWORD,
    GREAT_SWORD,
    BATTLE_AXE,
    GREAT_AXE,
    WAR_HAMMER,
    MACE,
    STAFF,
    
    // Weapons - Ranged
    LONG_BOW,
    SHORT_BOW,
    CROSSBOW,
    
    // Armor
    LEATHER_ARMOR,
    CHAIN_MAIL,
    PLATE_MAIL,
    
    // Shields
    SMALL_SHIELD,
    MEDIUM_SHIELD,
    LARGE_SHIELD,
    
    // Consumables
    HEALTH_POTION,
    MANA_POTION,
    INVISIBILITY_POTION,
    FOOD_RATION,
    BREAD,
    MEAT,
    FRUIT,
    
    // Scrolls
    SCROLL_LIGHTNING,
    SCROLL_FIREBALL,
    SCROLL_CONFUSION,
    SCROLL_TELEPORT,
    
    // Jewelry
    AMULET,
    RING,
    
    // Treasure
    GOLD,
    GEM,
    
    // Tools
    TORCH,
    ROPE,
    LOCKPICK,
    
    // Special/Quest Items
    AMULET_OF_YENDOR
};

// Item category for grouping
enum class ItemCategory
{
    UNKNOWN = 0,
    WEAPON,
    ARMOR,
    SHIELD,
    CONSUMABLE,
    SCROLL,
    JEWELRY,
    TREASURE,
    TOOL,
    QUEST_ITEM
};

// Utility functions for item classification
namespace ItemClassificationUtils
{
    // Get category from item class
    ItemCategory get_category(ItemClass itemClass);
    
    // Get display name for item class
    std::string get_display_name(ItemClass itemClass);
    
    // Get item class display name (same as above, for consistency)
    inline std::string get_item_display_name(ItemClass itemClass) { return get_display_name(itemClass); }
    
    // Type checking functions
    inline bool is_weapon(ItemClass itemClass) { return get_category(itemClass) == ItemCategory::WEAPON; }
    inline bool is_armor(ItemClass itemClass) { return get_category(itemClass) == ItemCategory::ARMOR; }
    inline bool is_shield(ItemClass itemClass) { return get_category(itemClass) == ItemCategory::SHIELD; }
    inline bool is_consumable(ItemClass itemClass) { return get_category(itemClass) == ItemCategory::CONSUMABLE; }
    inline bool is_scroll(ItemClass itemClass) { return get_category(itemClass) == ItemCategory::SCROLL; }
    inline bool is_jewelry(ItemClass itemClass) { return get_category(itemClass) == ItemCategory::JEWELRY; }
    inline bool is_treasure(ItemClass itemClass) { return get_category(itemClass) == ItemCategory::TREASURE; }
    inline bool is_tool(ItemClass itemClass) { return get_category(itemClass) == ItemCategory::TOOL; }
    inline bool is_quest_item(ItemClass itemClass) { return get_category(itemClass) == ItemCategory::QUEST_ITEM; }
    
    // Weapon sub-type checking
    bool is_ranged_weapon(ItemClass itemClass);
    
    // Equipment slot detection
    bool can_equip_to_right_hand(ItemClass itemClass);
    bool can_equip_to_left_hand(ItemClass itemClass);
    bool can_equip_to_body(ItemClass itemClass);
    bool is_two_handed_weapon(ItemClass itemClass);
    
    // Convert from string (for loading/creation)
    ItemClass from_string(const std::string& typeName);
    
    // Weapon size detection for D&D 2e mechanics
    WeaponSize get_weapon_size(ItemClass itemClass);
}

#endif // ITEM_CLASSIFICATION_H