// ItemClassification.cpp - Implementation of proper item classification system
#include "ItemClassification.h"
#include <unordered_map>

namespace ItemClassificationUtils
{
    ItemCategory get_category(ItemClass itemClass)
    {
        switch (itemClass)
        {
            // Weapons
            case ItemClass::DAGGER:
            case ItemClass::SWORD:
            case ItemClass::GREAT_SWORD:
            case ItemClass::AXE:
            case ItemClass::HAMMER:
            case ItemClass::MACE:
            case ItemClass::STAFF:
            case ItemClass::BOW:
            case ItemClass::CROSSBOW:
                return ItemCategory::WEAPON;

            // Armor
            case ItemClass::ARMOR:
                return ItemCategory::ARMOR;

            // Helmets
            case ItemClass::HELMET:
                return ItemCategory::HELMET;

            // Shields
            case ItemClass::SHIELD:
                return ItemCategory::SHIELD;

            // Gauntlets
            case ItemClass::GAUNTLETS:
                return ItemCategory::GAUNTLETS;

            // Girdles
            case ItemClass::GIRDLE:
                return ItemCategory::GIRDLE;

            // Consumables
            case ItemClass::POTION:
            case ItemClass::FOOD:
                return ItemCategory::CONSUMABLE;

            // Scrolls
            case ItemClass::SCROLL:
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
            case ItemClass::TOOL:
                return ItemCategory::TOOL;

            // Special/Quest Items
            case ItemClass::QUEST_ITEM:
                return ItemCategory::QUEST_ITEM;

            default:
                return ItemCategory::UNKNOWN;
        }
    }

    ItemClass get_class_from_id(ItemId itemId)
    {
        switch (itemId)
        {
            // Potions
            case ItemId::HEALTH_POTION:
            case ItemId::MANA_POTION:
            case ItemId::INVISIBILITY_POTION:
                return ItemClass::POTION;

            // Scrolls
            case ItemId::SCROLL_LIGHTNING:
            case ItemId::SCROLL_FIREBALL:
            case ItemId::SCROLL_CONFUSION:
            case ItemId::SCROLL_TELEPORT:
                return ItemClass::SCROLL;

            // Weapons - Daggers
            case ItemId::DAGGER:
                return ItemClass::DAGGER;

            // Weapons - Swords
            case ItemId::SHORT_SWORD:
            case ItemId::LONG_SWORD:
                return ItemClass::SWORD;

            // Weapons - Great Swords
            case ItemId::GREAT_SWORD:
                return ItemClass::GREAT_SWORD;

            // Weapons - Axes
            case ItemId::BATTLE_AXE:
            case ItemId::GREAT_AXE:
                return ItemClass::AXE;

            // Weapons - Hammers
            case ItemId::WAR_HAMMER:
                return ItemClass::HAMMER;

            // Weapons - Maces
            case ItemId::MACE:
                return ItemClass::MACE;

            // Weapons - Staves
            case ItemId::STAFF:
                return ItemClass::STAFF;

            // Weapons - Bows
            case ItemId::LONG_BOW:
            case ItemId::SHORT_BOW:
                return ItemClass::BOW;

            // Weapons - Crossbows
            case ItemId::CROSSBOW:
                return ItemClass::CROSSBOW;

            // Armor
            case ItemId::LEATHER_ARMOR:
            case ItemId::CHAIN_MAIL:
            case ItemId::PLATE_MAIL:
                return ItemClass::ARMOR;

            // Shields
            case ItemId::SMALL_SHIELD:
            case ItemId::MEDIUM_SHIELD:
            case ItemId::LARGE_SHIELD:
                return ItemClass::SHIELD;

            // Helmets
            case ItemId::HELM_OF_BRILLIANCE:
            case ItemId::HELM_OF_TELEPORTATION:
                return ItemClass::HELMET;

            // Rings
            case ItemId::RING_OF_PROTECTION_PLUS_1:
            case ItemId::RING_OF_PROTECTION_PLUS_2:
            case ItemId::RING_OF_FREE_ACTION:
            case ItemId::RING_OF_REGENERATION:
            case ItemId::RING_OF_INVISIBILITY:
                return ItemClass::RING;

            // Amulets
            case ItemId::AMULET_OF_HEALTH:
            case ItemId::AMULET_OF_WISDOM:
            case ItemId::AMULET_OF_PROTECTION:
                return ItemClass::AMULET;

            // Gauntlets
            case ItemId::GAUNTLETS_OF_OGRE_POWER:
            case ItemId::GAUNTLETS_OF_DEXTERITY:
                return ItemClass::GAUNTLETS;

            // Girdles
            case ItemId::GIRDLE_OF_HILL_GIANT_STRENGTH:
            case ItemId::GIRDLE_OF_FROST_GIANT_STRENGTH:
                return ItemClass::GIRDLE;

            // Food
            case ItemId::FOOD_RATION:
            case ItemId::BREAD:
            case ItemId::MEAT:
            case ItemId::FRUIT:
                return ItemClass::FOOD;

            // Treasure
            case ItemId::GOLD:
                return ItemClass::GOLD;
            case ItemId::GEM:
                return ItemClass::GEM;

            // Tools
            case ItemId::TORCH:
            case ItemId::ROPE:
            case ItemId::LOCKPICK:
                return ItemClass::TOOL;

            // Quest Items
            case ItemId::AMULET_OF_YENDOR:
                return ItemClass::QUEST_ITEM;

            default:
                return ItemClass::UNKNOWN;
        }
    }

    std::string get_display_name(ItemId itemId)
    {
        switch (itemId)
        {
            // Potions
            case ItemId::HEALTH_POTION: return "health potion";
            case ItemId::MANA_POTION: return "mana potion";
            case ItemId::INVISIBILITY_POTION: return "invisibility potion";

            // Scrolls
            case ItemId::SCROLL_LIGHTNING: return "lightning bolt scroll";
            case ItemId::SCROLL_FIREBALL: return "fireball scroll";
            case ItemId::SCROLL_CONFUSION: return "confusion scroll";
            case ItemId::SCROLL_TELEPORT: return "teleport scroll";

            // Weapons - Melee
            case ItemId::DAGGER: return "dagger";
            case ItemId::SHORT_SWORD: return "short sword";
            case ItemId::LONG_SWORD: return "long sword";
            case ItemId::GREAT_SWORD: return "great sword";
            case ItemId::BATTLE_AXE: return "battle axe";
            case ItemId::GREAT_AXE: return "great axe";
            case ItemId::WAR_HAMMER: return "war hammer";
            case ItemId::MACE: return "mace";
            case ItemId::STAFF: return "staff";

            // Weapons - Ranged
            case ItemId::LONG_BOW: return "long bow";
            case ItemId::SHORT_BOW: return "short bow";
            case ItemId::CROSSBOW: return "crossbow";

            // Armor
            case ItemId::LEATHER_ARMOR: return "leather armor";
            case ItemId::CHAIN_MAIL: return "chain mail";
            case ItemId::PLATE_MAIL: return "plate mail";

            // Shields
            case ItemId::SMALL_SHIELD: return "small shield";
            case ItemId::MEDIUM_SHIELD: return "shield";
            case ItemId::LARGE_SHIELD: return "large shield";

            // Helmets
            case ItemId::HELM_OF_BRILLIANCE: return "helm of brilliance";
            case ItemId::HELM_OF_TELEPORTATION: return "helm of teleportation";

            // Rings
            case ItemId::RING_OF_PROTECTION_PLUS_1: return "ring of protection +1";
            case ItemId::RING_OF_PROTECTION_PLUS_2: return "ring of protection +2";
            case ItemId::RING_OF_FREE_ACTION: return "ring of free action";
            case ItemId::RING_OF_REGENERATION: return "ring of regeneration";
            case ItemId::RING_OF_INVISIBILITY: return "ring of invisibility";

            // Amulets
            case ItemId::AMULET_OF_HEALTH: return "amulet of health";
            case ItemId::AMULET_OF_WISDOM: return "amulet of wisdom";
            case ItemId::AMULET_OF_PROTECTION: return "amulet of protection";
            case ItemId::AMULET_OF_YENDOR: return "Amulet of Yendor";

            // Gauntlets
            case ItemId::GAUNTLETS_OF_OGRE_POWER: return "gauntlets of ogre power";
            case ItemId::GAUNTLETS_OF_DEXTERITY: return "gauntlets of dexterity";

            // Girdles
            case ItemId::GIRDLE_OF_HILL_GIANT_STRENGTH: return "girdle of hill giant strength";
            case ItemId::GIRDLE_OF_FROST_GIANT_STRENGTH: return "girdle of frost giant strength";

            // Food
            case ItemId::FOOD_RATION: return "food ration";
            case ItemId::BREAD: return "bread";
            case ItemId::MEAT: return "meat";
            case ItemId::FRUIT: return "fruit";

            // Treasure
            case ItemId::GOLD: return "gold";
            case ItemId::GEM: return "gem";

            // Tools
            case ItemId::TORCH: return "torch";
            case ItemId::ROPE: return "rope";
            case ItemId::LOCKPICK: return "lockpick";

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
            case ItemClass::AXE:  // Great axes are two-handed
            case ItemClass::BOW:
            case ItemClass::CROSSBOW:
            case ItemClass::STAFF:
                return true;
            default:
                return false;
        }
    }

    ItemId item_id_from_string(const std::string& typeName)
    {
        static const std::unordered_map<std::string, ItemId> idMap = {
            // Potions
            {"health_potion", ItemId::HEALTH_POTION},
            {"mana_potion", ItemId::MANA_POTION},
            {"invisibility_potion", ItemId::INVISIBILITY_POTION},

            // Scrolls
            {"scroll_lightning", ItemId::SCROLL_LIGHTNING},
            {"scroll_fireball", ItemId::SCROLL_FIREBALL},
            {"scroll_confusion", ItemId::SCROLL_CONFUSION},
            {"scroll_teleport", ItemId::SCROLL_TELEPORT},

            // Weapons
            {"dagger", ItemId::DAGGER},
            {"short_sword", ItemId::SHORT_SWORD},
            {"long_sword", ItemId::LONG_SWORD},
            {"great_sword", ItemId::GREAT_SWORD},
            {"battle_axe", ItemId::BATTLE_AXE},
            {"great_axe", ItemId::GREAT_AXE},
            {"war_hammer", ItemId::WAR_HAMMER},
            {"mace", ItemId::MACE},
            {"staff", ItemId::STAFF},
            {"long_bow", ItemId::LONG_BOW},
            {"short_bow", ItemId::SHORT_BOW},
            {"crossbow", ItemId::CROSSBOW},

            // Armor
            {"leather_armor", ItemId::LEATHER_ARMOR},
            {"chain_mail", ItemId::CHAIN_MAIL},
            {"plate_mail", ItemId::PLATE_MAIL},

            // Shields
            {"small_shield", ItemId::SMALL_SHIELD},
            {"shield", ItemId::MEDIUM_SHIELD},
            {"large_shield", ItemId::LARGE_SHIELD},

            // Food
            {"food_ration", ItemId::FOOD_RATION},
            {"bread", ItemId::BREAD},
            {"meat", ItemId::MEAT},
            {"fruit", ItemId::FRUIT},

            // Treasure
            {"gold", ItemId::GOLD},
            {"gem", ItemId::GEM},

            // Tools
            {"torch", ItemId::TORCH},
            {"rope", ItemId::ROPE},
            {"lockpick", ItemId::LOCKPICK}
        };

        auto it = idMap.find(typeName);
        return (it != idMap.end()) ? it->second : ItemId::UNKNOWN;
    }

    ItemClass item_class_from_string(const std::string& typeName)
    {
        static const std::unordered_map<std::string, ItemClass> classMap = {
            // Weapons
            {"dagger", ItemClass::DAGGER},
            {"sword", ItemClass::SWORD},
            {"great_sword", ItemClass::GREAT_SWORD},
            {"axe", ItemClass::AXE},
            {"hammer", ItemClass::HAMMER},
            {"mace", ItemClass::MACE},
            {"staff", ItemClass::STAFF},
            {"bow", ItemClass::BOW},
            {"crossbow", ItemClass::CROSSBOW},

            // Armor & Protection
            {"armor", ItemClass::ARMOR},
            {"shield", ItemClass::SHIELD},
            {"helmet", ItemClass::HELMET},

            // Wearables
            {"ring", ItemClass::RING},
            {"amulet", ItemClass::AMULET},
            {"gauntlets", ItemClass::GAUNTLETS},
            {"girdle", ItemClass::GIRDLE},

            // Consumables
            {"potion", ItemClass::POTION},
            {"scroll", ItemClass::SCROLL},
            {"food", ItemClass::FOOD},

            // Other
            {"gold", ItemClass::GOLD},
            {"gem", ItemClass::GEM},
            {"tool", ItemClass::TOOL},
            {"quest_item", ItemClass::QUEST_ITEM}
        };

        auto it = classMap.find(typeName);
        return (it != classMap.end()) ? it->second : ItemClass::UNKNOWN;
    }

    WeaponSize get_weapon_size(ItemId itemId)
    {
        switch (itemId)
        {
            // TINY weapons - can always be off-hand
            case ItemId::DAGGER:
                return WeaponSize::TINY;

            // SMALL weapons - can be off-hand vs MEDIUM+ main hand
            case ItemId::SHORT_SWORD:
                return WeaponSize::SMALL;

            // MEDIUM weapons - main hand weapons
            case ItemId::LONG_SWORD:
            case ItemId::BATTLE_AXE:
            case ItemId::WAR_HAMMER:
            case ItemId::MACE:
                return WeaponSize::MEDIUM;

            // LARGE weapons - cannot dual wield (two-handed)
            case ItemId::GREAT_SWORD:
            case ItemId::GREAT_AXE:
            case ItemId::LONG_BOW:
            case ItemId::SHORT_BOW:
            case ItemId::CROSSBOW:
            case ItemId::STAFF:
                return WeaponSize::LARGE;

            // Non-weapons default to MEDIUM
            default:
                return WeaponSize::MEDIUM;
        }
    }
}
