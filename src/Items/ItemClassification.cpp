// ItemClassification.cpp - Implementation of proper item classification system
#include "ItemClassification.h"
#include <unordered_map>

namespace ItemClassificationUtils
{
    ItemCategory get_category(ItemClass itemClass)
    {
        switch (itemClass)
        {
            // Weapons - Melee
            case ItemClass::DAGGER:
            case ItemClass::SHORT_SWORD:
            case ItemClass::LONG_SWORD:
            case ItemClass::GREAT_SWORD:
            case ItemClass::BATTLE_AXE:
            case ItemClass::GREAT_AXE:
            case ItemClass::WAR_HAMMER:
            case ItemClass::MACE:
            case ItemClass::STAFF:
            // Weapons - Ranged
            case ItemClass::LONG_BOW:
            case ItemClass::SHORT_BOW:
            case ItemClass::CROSSBOW:
                return ItemCategory::WEAPON;
                
            // Armor
            case ItemClass::LEATHER_ARMOR:
            case ItemClass::CHAIN_MAIL:
            case ItemClass::PLATE_MAIL:
                return ItemCategory::ARMOR;

            // Helmets
            case ItemClass::HELMET:
                return ItemCategory::HELMET;

            // Shields
            case ItemClass::SMALL_SHIELD:
            case ItemClass::MEDIUM_SHIELD:
            case ItemClass::LARGE_SHIELD:
                return ItemCategory::SHIELD;

            // Gauntlets
            case ItemClass::GAUNTLETS:
                return ItemCategory::GAUNTLETS;

            // Girdles
            case ItemClass::GIRDLE:
                return ItemCategory::GIRDLE;

            // Consumables
            case ItemClass::HEALTH_POTION:
            case ItemClass::MANA_POTION:
            case ItemClass::INVISIBILITY_POTION:
            case ItemClass::FOOD_RATION:
            case ItemClass::BREAD:
            case ItemClass::MEAT:
            case ItemClass::FRUIT:
                return ItemCategory::CONSUMABLE;
                
            // Scrolls
            case ItemClass::SCROLL_LIGHTNING:
            case ItemClass::SCROLL_FIREBALL:
            case ItemClass::SCROLL_CONFUSION:
            case ItemClass::SCROLL_TELEPORT:
                return ItemCategory::SCROLL;
                
            // Jewelry
            case ItemClass::AMULET:
            case ItemClass::RING:
                return ItemCategory::JEWELRY;
                
            // Treasure
            case ItemClass::GOLD:
            case ItemClass::GEM:
                return ItemCategory::TREASURE;
                
            // Tools
            case ItemClass::TORCH:
            case ItemClass::ROPE:
            case ItemClass::LOCKPICK:
                return ItemCategory::TOOL;
                
            // Special/Quest Items
            case ItemClass::AMULET_OF_YENDOR:
                return ItemCategory::QUEST_ITEM;
                
            default:
                return ItemCategory::UNKNOWN;
        }
    }
    
    std::string get_display_name(ItemClass itemClass)
    {
        switch (itemClass)
        {
            // Weapons - Melee
            case ItemClass::DAGGER: return "dagger";
            case ItemClass::SHORT_SWORD: return "short sword";
            case ItemClass::LONG_SWORD: return "long sword";
            case ItemClass::GREAT_SWORD: return "great sword";
            case ItemClass::BATTLE_AXE: return "battle axe";
            case ItemClass::GREAT_AXE: return "great axe";
            case ItemClass::WAR_HAMMER: return "war hammer";
            case ItemClass::MACE: return "mace";
            case ItemClass::STAFF: return "staff";
            
            // Weapons - Ranged
            case ItemClass::LONG_BOW: return "long bow";
            case ItemClass::SHORT_BOW: return "short bow";
            case ItemClass::CROSSBOW: return "crossbow";
            
            // Armor
            case ItemClass::LEATHER_ARMOR: return "leather armor";
            case ItemClass::CHAIN_MAIL: return "chain mail";
            case ItemClass::PLATE_MAIL: return "plate mail";

            // Helmets
            case ItemClass::HELMET: return "helmet";

            // Shields
            case ItemClass::SMALL_SHIELD: return "small shield";
            case ItemClass::MEDIUM_SHIELD: return "shield";
            case ItemClass::LARGE_SHIELD: return "large shield";

            // Gauntlets
            case ItemClass::GAUNTLETS: return "gauntlets";

            // Girdles
            case ItemClass::GIRDLE: return "girdle";

            // Consumables
            case ItemClass::HEALTH_POTION: return "health potion";
            case ItemClass::MANA_POTION: return "mana potion";
            case ItemClass::INVISIBILITY_POTION: return "invisibility potion";
            case ItemClass::FOOD_RATION: return "food ration";
            case ItemClass::BREAD: return "bread";
            case ItemClass::MEAT: return "meat";
            case ItemClass::FRUIT: return "fruit";
            
            // Scrolls
            case ItemClass::SCROLL_LIGHTNING: return "lightning bolt scroll";
            case ItemClass::SCROLL_FIREBALL: return "fireball scroll";
            case ItemClass::SCROLL_CONFUSION: return "confusion scroll";
            case ItemClass::SCROLL_TELEPORT: return "teleport scroll";
            
            // Jewelry
            case ItemClass::AMULET: return "amulet";
            case ItemClass::RING: return "ring";
            
            // Treasure
            case ItemClass::GOLD: return "gold";
            case ItemClass::GEM: return "gem";
            
            // Tools
            case ItemClass::TORCH: return "torch";
            case ItemClass::ROPE: return "rope";
            case ItemClass::LOCKPICK: return "lockpick";
            
            // Special/Quest Items
            case ItemClass::AMULET_OF_YENDOR: return "Amulet of Yendor";
            
            default: return "unknown item";
        }
    }
    
    bool can_equip_to_right_hand(ItemClass itemClass)
    {
        return is_weapon(itemClass) || is_shield(itemClass);
    }
    
    bool can_equip_to_left_hand(ItemClass itemClass)
    {
        if (is_shield(itemClass)) return true;
        if (!is_weapon(itemClass)) return false;
        
        // Two-handed weapons cannot go in left hand
        return !is_two_handed_weapon(itemClass);
    }
    
    bool can_equip_to_body(ItemClass itemClass)
    {
        return is_armor(itemClass);
    }
    
    bool is_two_handed_weapon(ItemClass itemClass)
    {
        switch (itemClass)
        {
            case ItemClass::GREAT_SWORD:
            case ItemClass::GREAT_AXE:
            case ItemClass::LONG_BOW:
            case ItemClass::SHORT_BOW:
            case ItemClass::CROSSBOW:
            case ItemClass::STAFF: // Assuming staff is two-handed
                return true;
            default:
                return false;
        }
    }
    
    bool is_ranged_weapon(ItemClass itemClass)
    {
        switch (itemClass)
        {
            case ItemClass::LONG_BOW:
            case ItemClass::SHORT_BOW:
            case ItemClass::CROSSBOW:
                return true;
            default:
                return false;
        }
    }
    
    ItemClass from_string(const std::string& typeName)
    {
        static const std::unordered_map<std::string, ItemClass> classMap = {
            // Weapons - Melee
            {"dagger", ItemClass::DAGGER},
            {"short_sword", ItemClass::SHORT_SWORD},
            {"long_sword", ItemClass::LONG_SWORD},
            {"great_sword", ItemClass::GREAT_SWORD},
            {"battle_axe", ItemClass::BATTLE_AXE},
            {"great_axe", ItemClass::GREAT_AXE},
            {"war_hammer", ItemClass::WAR_HAMMER},
            {"mace", ItemClass::MACE},
            {"staff", ItemClass::STAFF},
            
            // Weapons - Ranged
            {"long_bow", ItemClass::LONG_BOW},
            {"short_bow", ItemClass::SHORT_BOW},
            {"crossbow", ItemClass::CROSSBOW},
            
            // Armor
            {"leather_armor", ItemClass::LEATHER_ARMOR},
            {"chain_mail", ItemClass::CHAIN_MAIL},
            {"plate_mail", ItemClass::PLATE_MAIL},
            
            // Shields
            {"small_shield", ItemClass::SMALL_SHIELD},
            {"shield", ItemClass::MEDIUM_SHIELD},
            {"large_shield", ItemClass::LARGE_SHIELD},

            // Gauntlets
            {"gauntlets", ItemClass::GAUNTLETS},

            // Girdles
            {"girdle", ItemClass::GIRDLE},

            // Consumables
            {"health_potion", ItemClass::HEALTH_POTION},
            {"mana_potion", ItemClass::MANA_POTION},
            {"invisibility_potion", ItemClass::INVISIBILITY_POTION},
            {"food_ration", ItemClass::FOOD_RATION},
            {"bread", ItemClass::BREAD},
            {"meat", ItemClass::MEAT},
            {"fruit", ItemClass::FRUIT},
            
            // Scrolls
            {"scroll_lightning", ItemClass::SCROLL_LIGHTNING},
            {"scroll_fireball", ItemClass::SCROLL_FIREBALL},
            {"scroll_confusion", ItemClass::SCROLL_CONFUSION},
            {"scroll_teleport", ItemClass::SCROLL_TELEPORT},
            
            // Jewelry
            {"amulet", ItemClass::AMULET},
            {"ring", ItemClass::RING},
            
            // Treasure
            {"gold", ItemClass::GOLD},
            {"gem", ItemClass::GEM},
            
            // Tools
            {"torch", ItemClass::TORCH},
            {"rope", ItemClass::ROPE},
            {"lockpick", ItemClass::LOCKPICK},
            
            // Special/Quest Items
            {"amulet_of_yendor", ItemClass::AMULET_OF_YENDOR}
        };
        
        auto it = classMap.find(typeName);
        return (it != classMap.end()) ? it->second : ItemClass::UNKNOWN;
    }
    
    WeaponSize get_weapon_size(ItemClass itemClass)
    {
        switch (itemClass)
        {
            // TINY weapons - can always be off-hand
            case ItemClass::DAGGER:
                return WeaponSize::TINY;
                
            // SMALL weapons - can be off-hand vs MEDIUM+ main hand
            case ItemClass::SHORT_SWORD:
                return WeaponSize::SMALL;
                
            // MEDIUM weapons - main hand weapons
            case ItemClass::LONG_SWORD:
            case ItemClass::BATTLE_AXE:
            case ItemClass::WAR_HAMMER:
            case ItemClass::MACE:
                return WeaponSize::MEDIUM;
                
            // LARGE weapons - cannot dual wield (two-handed)
            case ItemClass::GREAT_SWORD:
            case ItemClass::GREAT_AXE:
            case ItemClass::LONG_BOW:
            case ItemClass::SHORT_BOW:
            case ItemClass::CROSSBOW:
            case ItemClass::STAFF:
                return WeaponSize::LARGE;
                
            // Non-weapons default to MEDIUM
            default:
                return WeaponSize::MEDIUM;
        }
    }
}