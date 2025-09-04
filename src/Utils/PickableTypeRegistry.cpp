// PickableTypeRegistry.cpp - Implementation of type registry
#include "PickableTypeRegistry.h"
#include "../Actor/Actor.h"
#include "../ActorTypes/Gold.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/Fireball.h"
#include "../ActorTypes/Confuser.h"
#include "../Items/Weapons.h"
#include "../Items/Armor.h"

namespace PickableTypeRegistry
{
    Type get_type(const Pickable* pickable)
    {
        if (!pickable) return Type::UNKNOWN;
        
        // Use the existing get_type() method from Pickable
        switch (pickable->get_type())
        {
            case Pickable::PickableType::HEALER: return Type::HEALER;
            case Pickable::PickableType::LIGHTNING_BOLT: return Type::LIGHTNING_BOLT;
            case Pickable::PickableType::CONFUSER: return Type::CONFUSER;
            case Pickable::PickableType::FIREBALL: return Type::FIREBALL;
            case Pickable::PickableType::LONGSWORD: return Type::LONGSWORD;
            case Pickable::PickableType::DAGGER: return Type::DAGGER;
            case Pickable::PickableType::SHORTSWORD: return Type::SHORTSWORD;
            case Pickable::PickableType::LONGBOW: return Type::LONGBOW;
            case Pickable::PickableType::STAFF: return Type::STAFF;
            case Pickable::PickableType::GREATSWORD: return Type::GREATSWORD;
            case Pickable::PickableType::BATTLE_AXE: return Type::BATTLE_AXE;
            case Pickable::PickableType::GREAT_AXE: return Type::GREAT_AXE;
            case Pickable::PickableType::WAR_HAMMER: return Type::WAR_HAMMER;
            case Pickable::PickableType::SHIELD: return Type::SHIELD;
            case Pickable::PickableType::GOLD: return Type::GOLD;
            case Pickable::PickableType::FOOD: return Type::FOOD;
            case Pickable::PickableType::CORPSE_FOOD: return Type::CORPSE_FOOD;
            case Pickable::PickableType::AMULET: return Type::AMULET;
            case Pickable::PickableType::LEATHER_ARMOR: return Type::LEATHER_ARMOR;
            case Pickable::PickableType::CHAIN_MAIL: return Type::CHAIN_MAIL;
            case Pickable::PickableType::PLATE_MAIL: return Type::PLATE_MAIL;
            default: return Type::UNKNOWN;
        }
    }
    
    Category get_category(Type type)
    {
        switch (type)
        {
            case Type::LONGSWORD:
            case Type::DAGGER:
            case Type::SHORTSWORD:
            case Type::LONGBOW:
            case Type::STAFF:
            case Type::GREATSWORD:
            case Type::BATTLE_AXE:
            case Type::GREAT_AXE:
            case Type::WAR_HAMMER:
            case Type::SHIELD:
                return Category::WEAPON;
                
            case Type::LEATHER_ARMOR:
            case Type::CHAIN_MAIL:
            case Type::PLATE_MAIL:
                return Category::ARMOR;
                
            case Type::HEALER:
            case Type::FOOD:
            case Type::CORPSE_FOOD:
                return Category::CONSUMABLE;
                
            case Type::GOLD:
            case Type::AMULET:
                return Category::TREASURE;
                
            case Type::LIGHTNING_BOLT:
            case Type::CONFUSER:
            case Type::FIREBALL:
                return Category::TOOL;
                
            default:
                return Category::UNKNOWN;
        }
    }
    
    std::string get_display_name(Type type)
    {
        switch (type)
        {
            case Type::HEALER: return "Health Potion";
            case Type::LIGHTNING_BOLT: return "Lightning Bolt Scroll";
            case Type::CONFUSER: return "Confusion Scroll";
            case Type::FIREBALL: return "Fireball Scroll";
            case Type::LONGSWORD: return "Long Sword";
            case Type::DAGGER: return "Dagger";
            case Type::SHORTSWORD: return "Short Sword";
            case Type::LONGBOW: return "Long Bow";
            case Type::STAFF: return "Staff";
            case Type::GREATSWORD: return "Great Sword";
            case Type::BATTLE_AXE: return "Battle Axe";
            case Type::GREAT_AXE: return "Great Axe";
            case Type::WAR_HAMMER: return "War Hammer";
            case Type::SHIELD: return "Shield";
            case Type::GOLD: return "Gold";
            case Type::FOOD: return "Food";
            case Type::CORPSE_FOOD: return "Corpse Food";
            case Type::AMULET: return "Amulet";
            case Type::LEATHER_ARMOR: return "Leather Armor";
            case Type::CHAIN_MAIL: return "Chain Mail";
            case Type::PLATE_MAIL: return "Plate Mail";
            default: return "Unknown Item";
        }
    }
    
    Type get_item_type(const Item& item)
    {
        return get_type(item.pickable.get());
    }
    
    Type get_weapon_type(const Weapon* weapon)
    {
        return get_type(weapon);
    }
    
    WeaponSize get_weapon_size(Type type)
    {
        switch (type)
        {
            case Type::DAGGER:
                return WeaponSize::TINY;
                
            case Type::SHORTSWORD:
                return WeaponSize::SMALL;
                
            case Type::LONGSWORD:
            case Type::BATTLE_AXE:
            case Type::WAR_HAMMER:
            case Type::STAFF:
                return WeaponSize::MEDIUM;
                
            case Type::GREATSWORD:
            case Type::GREAT_AXE:
                return WeaponSize::LARGE;
                
            default:
                return WeaponSize::MEDIUM; // Default to MEDIUM
        }
    }
}
