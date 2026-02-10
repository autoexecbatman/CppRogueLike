#pragma once

#include <string>
#include "Weapons.h" // For WeaponSize enum

// IDENTITY - Unique identifier for each specific item variant
enum class ItemId
{
    UNKNOWN = 0,

    // Potions
    HEALTH_POTION,
    POTION_OF_EXTRA_HEALING,
    MANA_POTION,
    INVISIBILITY_POTION,
    POTION_OF_GIANT_STRENGTH,
    POTION_OF_LEVITATION,
    POTION_OF_FIRE_RESISTANCE,
    POTION_OF_COLD_RESISTANCE,
    POTION_OF_SPEED,

    // Scrolls
    SCROLL_LIGHTNING,
    SCROLL_FIREBALL,
    SCROLL_CONFUSION,
    SCROLL_TELEPORT,

    // Weapons - Melee
    DAGGER,
    SHORT_SWORD,
    LONG_SWORD,
    BASTARD_SWORD,
    TWO_HANDED_SWORD,
    GREAT_SWORD,
    SCIMITAR,
    RAPIER,
    HAND_AXE,
    BATTLE_AXE,
    GREAT_AXE,
    WAR_HAMMER,
    MACE,
    MORNING_STAR,
    FLAIL,
    CLUB,
    QUARTERSTAFF,
    STAFF,

    // Weapons - Ranged
    SHORT_BOW,
    LONG_BOW,
    COMPOSITE_BOW,
    LIGHT_CROSSBOW,
    HEAVY_CROSSBOW,
    CROSSBOW,
    SLING,

    // Armor
    PADDED_ARMOR,
    LEATHER_ARMOR,
    STUDDED_LEATHER,
    HIDE_ARMOR,
    RING_MAIL,
    SCALE_MAIL,
    CHAIN_MAIL,
    BRIGANDINE,
    SPLINT_MAIL,
    BANDED_MAIL,
    PLATE_MAIL,
    FIELD_PLATE,
    FULL_PLATE,

    // Shields
    SMALL_SHIELD,
    MEDIUM_SHIELD,
    LARGE_SHIELD,

    // Helmets
    HELM_OF_BRILLIANCE,
    HELM_OF_TELEPORTATION,
    HELM_OF_TELEPATHY,
    HELM_OF_UNDERWATER_ACTION,

    // Rings
    RING_OF_PROTECTION_PLUS_1,
    RING_OF_PROTECTION_PLUS_2,
    RING_OF_FREE_ACTION,
    RING_OF_REGENERATION,
    RING_OF_INVISIBILITY,
    RING_OF_FIRE_RESISTANCE,
    RING_OF_COLD_RESISTANCE,
    RING_OF_SPELL_STORING,

    // Amulets
    AMULET_OF_HEALTH,
    AMULET_OF_WISDOM,
    AMULET_OF_PROTECTION,
    AMULET_OF_OGRE_POWER,
    AMULET_OF_YENDOR,

    // Gauntlets
    GAUNTLETS_OF_OGRE_POWER,
    GAUNTLETS_OF_DEXTERITY,
    GAUNTLETS_OF_SWIMMING_AND_CLIMBING,
    GAUNTLETS_OF_FUMBLING,

    // Girdles
    GIRDLE_OF_HILL_GIANT_STRENGTH,
    GIRDLE_OF_STONE_GIANT_STRENGTH,
    GIRDLE_OF_FROST_GIANT_STRENGTH,
    GIRDLE_OF_FIRE_GIANT_STRENGTH,
    GIRDLE_OF_CLOUD_GIANT_STRENGTH,
    GIRDLE_OF_STORM_GIANT_STRENGTH,

    // Boots
    BOOTS_OF_SPEED,
    BOOTS_OF_ELVENKIND,
    BOOTS_OF_LEVITATION,

    // Cloaks
    CLOAK_OF_PROTECTION,
    CLOAK_OF_DISPLACEMENT,
    CLOAK_OF_ELVENKIND,

    // Food
    FOOD_RATION,
    BREAD,
    MEAT,
    FRUIT,

    // Treasure
    GOLD,
    GEM,

    // Tools
    TORCH,
    ROPE,
    LOCKPICK,
};

// CLASSIFICATION - Category/type of item (no specific variants)
enum class ItemClass
{
    UNKNOWN = 0,

    // Weapons
    DAGGER,
    SWORD,
    GREAT_SWORD,
    AXE,
    HAMMER,
    MACE,
    STAFF,
    BOW,
    CROSSBOW,

    // Armor & Protection
    ARMOR,
    SHIELD,
    HELMET,

    // Wearables
    RING,
    AMULET,
    GAUNTLETS,
    GIRDLE,

    // Consumables
    POTION,
    SCROLL,
    FOOD,

    // Other
    GOLD,
    GEM,
    TOOL,
    QUEST_ITEM,
};

// Item category for grouping
enum class ItemCategory
{
    UNKNOWN = 0,
    WEAPON,
    ARMOR,
    HELMET,
    SHIELD,
    GAUNTLETS,
    GIRDLE,
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

    // Get ItemClass from ItemId
    ItemClass get_class_from_id(ItemId itemId);

    // Get display name for item ID
    std::string get_display_name(ItemId itemId);

    // Type checking functions - now simple and clean
    inline bool is_weapon(ItemClass itemClass)
    {
        return itemClass == ItemClass::DAGGER ||
               itemClass == ItemClass::SWORD ||
               itemClass == ItemClass::GREAT_SWORD ||
               itemClass == ItemClass::AXE ||
               itemClass == ItemClass::HAMMER ||
               itemClass == ItemClass::MACE ||
               itemClass == ItemClass::STAFF ||
               itemClass == ItemClass::BOW ||
               itemClass == ItemClass::CROSSBOW;
    }

    inline bool is_armor(ItemClass itemClass) { return itemClass == ItemClass::ARMOR; }
    inline bool is_helmet(ItemClass itemClass) { return itemClass == ItemClass::HELMET; }
    inline bool is_shield(ItemClass itemClass) { return itemClass == ItemClass::SHIELD; }
    inline bool is_gauntlets(ItemClass itemClass) { return itemClass == ItemClass::GAUNTLETS; }
    inline bool is_girdle(ItemClass itemClass) { return itemClass == ItemClass::GIRDLE; }
    inline bool is_potion(ItemClass itemClass) { return itemClass == ItemClass::POTION; }
    inline bool is_scroll(ItemClass itemClass) { return itemClass == ItemClass::SCROLL; }
    inline bool is_food(ItemClass itemClass) { return itemClass == ItemClass::FOOD; }
    inline bool is_amulet(ItemClass itemClass) { return itemClass == ItemClass::AMULET; }
    inline bool is_ring(ItemClass itemClass) { return itemClass == ItemClass::RING; }
    inline bool is_treasure(ItemClass itemClass) { return itemClass == ItemClass::GOLD || itemClass == ItemClass::GEM; }
    inline bool is_tool(ItemClass itemClass) { return itemClass == ItemClass::TOOL; }
    inline bool is_quest_item(ItemClass itemClass) { return itemClass == ItemClass::QUEST_ITEM; }

    // Composite type checking
    inline bool is_consumable(ItemClass itemClass) { return is_potion(itemClass) || is_scroll(itemClass) || is_food(itemClass); }
    inline bool is_jewelry(ItemClass itemClass) { return is_ring(itemClass) || is_amulet(itemClass); }

    // Ranged weapon checking
    inline bool is_ranged_weapon(ItemClass itemClass)
    {
        return itemClass == ItemClass::BOW || itemClass == ItemClass::CROSSBOW;
    }

    // Equipment slot detection
    bool can_equip_to_right_hand(ItemClass itemClass);
    bool can_equip_to_left_hand(ItemClass itemClass);
    bool can_equip_to_body(ItemClass itemClass);
    bool is_two_handed_weapon(ItemClass itemClass);

    // Convert from string (for loading/creation)
    ItemId item_id_from_string(const std::string& typeName);
    ItemClass item_class_from_string(const std::string& typeName);

    // Weapon size detection for D&D 2e mechanics
    WeaponSize get_weapon_size(ItemId itemId);
}
