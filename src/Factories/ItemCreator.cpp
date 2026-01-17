#include "ItemCreator.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/Fireball.h"
#include "../ActorTypes/Confuser.h"
#include "../ActorTypes/Teleporter.h"
#include "../Actor/Pickable.h"
#include "../Items/Armor.h"
#include "../Colors/Colors.h"
#include "../Items/Weapons.h"
#include "../ActorTypes/Gold.h"
#include "../Items/Food.h"
#include "../Items/Amulet.h"
#include "../Random/RandomDice.h"
#include "../Core/GameContext.h"

// UNIFIED ITEM CREATION - HARDCODED DATA IN ONE PLACE

// Potions - Use existing working classes
std::unique_ptr<Item> ItemCreator::create_health_potion(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'!', "health potion", WHITE_RED_PAIR});
    item->pickable = std::make_unique<Healer>(10);  // Hardcoded heal amount
    item->set_value(50);  // Hardcoded value
    return item;
}

// Scrolls - Use existing working classes with hardcoded values
std::unique_ptr<Item> ItemCreator::create_scroll_lightning(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'?', "scroll of lightning bolt", WHITE_BLUE_PAIR});
    item->pickable = std::make_unique<LightningBolt>(5, 20);  // Hardcoded range, damage
    item->set_value(150);  // Hardcoded value
    return item;
}

std::unique_ptr<Item> ItemCreator::create_scroll_fireball(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'?', "scroll of fireball", RED_YELLOW_PAIR});
    item->pickable = std::make_unique<Fireball>(3, 12);  // Hardcoded range, damage
    item->set_value(100);  // Hardcoded value
    return item;
}

std::unique_ptr<Item> ItemCreator::create_scroll_confusion(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'?', "scroll of confusion", WHITE_GREEN_PAIR});
    item->pickable = std::make_unique<Confuser>(10, 8);  // Hardcoded range, turns
    item->set_value(120);  // Hardcoded value
    return item;
}


std::unique_ptr<Item> ItemCreator::create_invisibility_potion(Vector2D position)
{
    auto item = std::make_unique<Item>(position, ActorData{'!', "Invisibility Potion", CYAN_BLACK_PAIR});
    item->pickable = std::make_unique<InvisibilityPotion>(30);
    item->itemClass = ItemClass::INVISIBILITY_POTION;
    item->set_value(150);
    return item;
}


std::unique_ptr<Item> ItemCreator::create_scroll_teleportation(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'?', "scroll of teleportation", MAGENTA_BLACK_PAIR});
    item->pickable = std::make_unique<Teleporter>();
    item->set_value(200);
    return item;
}

// Random item generation using unified methods
std::unique_ptr<Item> ItemCreator::create_random_potion(Vector2D pos, int dungeonLevel)
{
    // Use unified potion creation
    return create_health_potion(pos);
}

std::unique_ptr<Item> ItemCreator::create_random_scroll(Vector2D pos, GameContext& ctx, int dungeonLevel)
{
    int roll = ctx.dice->roll(1, 4);
    switch(roll)
    {
        case 1: return create_scroll_lightning(pos);
        case 2: return create_scroll_fireball(pos);
        case 3: return create_scroll_confusion(pos);
        case 4: return create_scroll_teleportation(pos);
        default: return create_scroll_lightning(pos);
    }
}

std::unique_ptr<Item> ItemCreator::create_dagger(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "dagger", WHITE_BLACK_PAIR});
    item->itemClass = ItemClass::DAGGER;
    create_pickable_from_itemclass(item.get(), ItemClass::DAGGER);
    item->set_value(2); // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_short_sword(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "short sword", WHITE_BLACK_PAIR});
    item->itemClass = ItemClass::SHORT_SWORD;
    create_pickable_from_itemclass(item.get(), ItemClass::SHORT_SWORD);
    item->set_value(10); // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_long_sword(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "long sword", WHITE_BLACK_PAIR});
    item->itemClass = ItemClass::LONG_SWORD;
    create_pickable_from_itemclass(item.get(), ItemClass::LONG_SWORD);
    item->set_value(15); // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_staff(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "staff", WHITE_BLACK_PAIR});
    item->pickable = std::make_unique<Staff>();
    item->itemClass = ItemClass::STAFF;
    item->set_value(6); // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_longbow(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{')', "longbow", WHITE_BLUE_PAIR});
    item->pickable = std::make_unique<Longbow>();
    item->itemClass = ItemClass::LONG_BOW;
    item->set_value(75); // AD&D 2e price
    return item;
}

// Two-handed weapon creation functions
std::unique_ptr<Item> ItemCreator::create_greatsword(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "greatsword", WHITE_BLACK_PAIR});
    item->pickable = std::make_unique<Greatsword>();
    item->itemClass = ItemClass::GREAT_SWORD; // Fix: Set proper item classification
    item->set_value(50);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_battle_axe(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "battle axe", WHITE_BLACK_PAIR});
    item->pickable = std::make_unique<BattleAxe>();
    item->itemClass = ItemClass::BATTLE_AXE;
    item->set_value(25);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_great_axe(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "great axe", WHITE_BLACK_PAIR});
    item->pickable = std::make_unique<GreatAxe>();
    item->itemClass = ItemClass::GREAT_AXE;
    item->set_value(40);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_war_hammer(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "war hammer", WHITE_BLACK_PAIR});
    item->pickable = std::make_unique<WarHammer>();
    item->itemClass = ItemClass::WAR_HAMMER;
    item->set_value(20);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_mace(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'|', "mace", WHITE_BLACK_PAIR});
    item->itemClass = ItemClass::MACE;
    create_pickable_from_itemclass(item.get(), ItemClass::MACE);
    item->set_value(8);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_shield(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'[', "shield", WHITE_BLACK_PAIR});
    item->pickable = std::make_unique<Shield>();
    item->itemClass = ItemClass::MEDIUM_SHIELD;
    item->set_value(10); // AD&D 2e price
    return item;
}

// Enhanced weapon creation functions
std::unique_ptr<Item> ItemCreator::create_enhanced_dagger(Vector2D pos, int enhancementLevel)
{
    auto item = create_dagger(pos);
    if (enhancementLevel > 0)
    {
        // Create and apply enhancement based on enhancement level
        ItemEnhancement enhancement;
        enhancement.damage_bonus = enhancementLevel;
        enhancement.prefix = (enhancementLevel == 1) ? PrefixType::SHARP : PrefixType::KEEN;
        enhancement.apply_enhancement_effects();

        item->apply_enhancement(enhancement);

        // Enhanced weapons get visual distinction
        item->actorData.color = WHITE_GREEN_PAIR;
    }
    return item;
}

std::unique_ptr<Item> ItemCreator::create_enhanced_short_sword(Vector2D pos, int enhancementLevel)
{
    auto item = create_short_sword(pos);
    if (enhancementLevel > 0)
    {
        // Create traditional "+X" enhancement
        ItemEnhancement enhancement;
        enhancement.enhancement_level = enhancementLevel;
        enhancement.damage_bonus = enhancementLevel;
        enhancement.to_hit_bonus = enhancementLevel;
        enhancement.value_modifier = 100 + (enhancementLevel * 50); // +50% per level
        enhancement.apply_enhancement_effects();

        item->apply_enhancement(enhancement);
        item->actorData.color = WHITE_GREEN_PAIR;
    }
    return item;
}

std::unique_ptr<Item> ItemCreator::create_enhanced_long_sword(Vector2D pos, int enhancementLevel)
{
    auto item = create_long_sword(pos);
    if (enhancementLevel > 0)
    {
        // Create traditional "+X" enhancement
        ItemEnhancement enhancement;
        enhancement.enhancement_level = enhancementLevel;
        enhancement.damage_bonus = enhancementLevel;
        enhancement.to_hit_bonus = enhancementLevel;
        enhancement.value_modifier = 100 + (enhancementLevel * 50); // +50% per level
        enhancement.apply_enhancement_effects();
        item->apply_enhancement(enhancement);
        item->actorData.color = WHITE_GREEN_PAIR;
    }
    return item;
}

std::unique_ptr<Item> ItemCreator::create_enhanced_staff(Vector2D pos, int enhancementLevel)
{
    auto item = create_staff(pos);
    if (enhancementLevel > 0)
    {
        // Create traditional "+X" enhancement
        ItemEnhancement enhancement;
        enhancement.enhancement_level = enhancementLevel;
        enhancement.damage_bonus = enhancementLevel;
        enhancement.to_hit_bonus = enhancementLevel;
        enhancement.value_modifier = 100 + (enhancementLevel * 50); // +50% per level
        enhancement.apply_enhancement_effects();
        item->apply_enhancement(enhancement);
        item->actorData.color = WHITE_GREEN_PAIR;
    }
    return item;
}

std::unique_ptr<Item> ItemCreator::create_enhanced_longbow(Vector2D pos, int enhancementLevel)
{
    auto item = create_longbow(pos);
    if (enhancementLevel > 0)
    {
        // Create traditional "+X" enhancement
        ItemEnhancement enhancement;
        enhancement.enhancement_level = enhancementLevel;
        enhancement.damage_bonus = enhancementLevel;
        enhancement.to_hit_bonus = enhancementLevel;
        enhancement.value_modifier = 100 + (enhancementLevel * 50); // +50% per level
        enhancement.apply_enhancement_effects();
        item->apply_enhancement(enhancement);
        item->actorData.color = WHITE_GREEN_PAIR;
    }
    return item;
}

// Random enhancement chance functions
std::unique_ptr<Item> ItemCreator::create_random_weapon(Vector2D pos, GameContext& ctx, int dungeonLevel)
{
    // Random weapon type selection
    int weaponType = ctx.dice->roll(1, 5);
    
    switch (weaponType)
    {
        case 1: return create_weapon_with_enhancement_chance(pos, ctx, dungeonLevel); // Dagger
        case 2: return create_enhanced_short_sword(pos, determine_enhancement_level(ctx, dungeonLevel));
        case 3: return create_enhanced_long_sword(pos, determine_enhancement_level(ctx, dungeonLevel));
        case 4: return create_enhanced_staff(pos, determine_enhancement_level(ctx, dungeonLevel));
        case 5: return create_enhanced_longbow(pos, determine_enhancement_level(ctx, dungeonLevel));
        default: return create_enhanced_dagger(pos, determine_enhancement_level(ctx, dungeonLevel));
    }
}

std::unique_ptr<Item> ItemCreator::create_weapon_with_enhancement_chance(Vector2D pos, GameContext& ctx, int dungeonLevel)
{
    int enhancementChance = calculate_enhancement_chance(dungeonLevel);
    int roll = ctx.dice->roll(1, 100);
    
    if (roll <= enhancementChance)
    {
        int enhancementLevel = determine_enhancement_level(ctx, dungeonLevel);
        return create_enhanced_dagger(pos, enhancementLevel);
    }
    else
    {
        return create_dagger(pos);
    }
}

// Enhancement utility functions
int ItemCreator::calculate_enhancement_chance(int dungeonLevel)
{
    // Base 5% chance, increases by 3% per dungeon level
    // Level 1: 5%, Level 2: 8%, Level 3: 11%, etc.
    // Caps at 35% chance at level 10+
    return std::min(5 + (dungeonLevel - 1) * 3, 35);
}

int ItemCreator::determine_enhancement_level(GameContext& ctx, int dungeonLevel)
{
    int roll = ctx.dice->roll(1, 100);
    
    // Higher dungeon levels get better enhancement chances
    if (dungeonLevel >= 5)
    {
        if (roll <= 10) return 3;      // 10% chance for +3
        if (roll <= 30) return 2;      // 20% chance for +2
        return 1;                      // 70% chance for +1
    }
    else if (dungeonLevel >= 3)
    {
        if (roll <= 20) return 2;      // 20% chance for +2
        return 1;                      // 80% chance for +1
    }
    else
    {
        return 1;                      // 100% chance for +1 on early levels
    }
}

int ItemCreator::calculate_enhanced_value(int baseValue, int enhancementLevel)
{
    // Each enhancement level doubles the base value
    return baseValue * (1 + enhancementLevel * 2);
}

std::unique_ptr<Item> ItemCreator::create_leather_armor(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'[', "leather armor", BROWN_BLACK_PAIR});
    item->pickable = std::make_unique<LeatherArmor>();
    item->itemClass = ItemClass::LEATHER_ARMOR;
    item->set_value(5); // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_chain_mail(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'[', "chain mail", BROWN_BLACK_PAIR});
    item->pickable = std::make_unique<ChainMail>();
    item->itemClass = ItemClass::CHAIN_MAIL;
    item->set_value(75); // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_plate_mail(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'[', "plate mail", BROWN_BLACK_PAIR});
    item->pickable = std::make_unique<PlateMail>();
    item->itemClass = ItemClass::PLATE_MAIL;
    item->set_value(400); // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_gold_pile(Vector2D pos, GameContext& ctx)
{
    auto item = std::make_unique<Item>(pos, ActorData{'$', "gold pile", YELLOW_BLACK_PAIR});
    int goldAmount = ctx.dice->roll(10, 50); // Random amount 10-50 gold
    item->pickable = std::make_unique<Gold>(goldAmount);
    item->set_value(goldAmount); // Gold's value is its amount
    return item;
}

// Food creation functions
std::unique_ptr<Item> ItemCreator::create_ration(Vector2D pos)
{
    return std::make_unique<Ration>(pos);
}

std::unique_ptr<Item> ItemCreator::create_fruit(Vector2D pos)
{
    return std::make_unique<Fruit>(pos);
}

std::unique_ptr<Item> ItemCreator::create_bread(Vector2D pos)
{
    return std::make_unique<Bread>(pos);
}

std::unique_ptr<Item> ItemCreator::create_meat(Vector2D pos)
{
    return std::make_unique<Meat>(pos);
}

// Artifact creation functions
std::unique_ptr<Item> ItemCreator::create_amulet_of_yendor(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'"', "Amulet of Yendor", YELLOW_BLACK_PAIR});
    item->pickable = std::make_unique<Amulet>();
    item->set_value(50000); // Priceless artifact
    return item;
}

// Factory method to create weapons by ItemClass
std::unique_ptr<Item> ItemCreator::create_weapon_by_class(Vector2D pos, ItemClass weaponClass)
{
    switch (weaponClass)
    {
        case ItemClass::DAGGER:      return create_dagger(pos);
        case ItemClass::SHORT_SWORD: return create_short_sword(pos);
        case ItemClass::LONG_SWORD:  return create_long_sword(pos);
        case ItemClass::STAFF:       return create_staff(pos);
        case ItemClass::BATTLE_AXE:  return create_battle_axe(pos);
        case ItemClass::GREAT_AXE:   return create_great_axe(pos);
        case ItemClass::WAR_HAMMER:  return create_war_hammer(pos);
        case ItemClass::MACE:        return create_mace(pos);
        case ItemClass::GREAT_SWORD: return create_greatsword(pos);
        case ItemClass::LONG_BOW:    return create_longbow(pos);
        default:                     return create_long_sword(pos); // Safe default
    }
}

// Proper ItemClass to Pickable mapping (no string comparisons)
void ItemCreator::create_pickable_from_itemclass(Item* item, ItemClass itemClass)
{
    switch (itemClass)
    {
        case ItemClass::DAGGER:      item->pickable = std::make_unique<Dagger>(); break;
        case ItemClass::SHORT_SWORD: item->pickable = std::make_unique<ShortSword>(); break;
        case ItemClass::LONG_SWORD:  item->pickable = std::make_unique<LongSword>(); break;
        case ItemClass::STAFF:       item->pickable = std::make_unique<Staff>(); break;
        case ItemClass::BATTLE_AXE:  item->pickable = std::make_unique<BattleAxe>(); break;
        case ItemClass::GREAT_AXE:   item->pickable = std::make_unique<GreatAxe>(); break;
        case ItemClass::WAR_HAMMER:  item->pickable = std::make_unique<WarHammer>(); break;
        case ItemClass::MACE:        item->pickable = std::make_unique<Mace>(); break;
        case ItemClass::GREAT_SWORD: item->pickable = std::make_unique<Greatsword>(); break;
        case ItemClass::LONG_BOW:    item->pickable = std::make_unique<Longbow>(); break;
        default:                     item->pickable = std::make_unique<LongSword>(); break; // Safe default
    }
}

void ItemCreator::ensure_correct_value(Item& item)
{
    // Fix items with incorrect values using the ItemClass system
    ItemClass itemClass = item.itemClass;
    
    // Set correct values based on item class (what the item IS)
    switch (itemClass)
    {
    case ItemClass::HEALTH_POTION:
        if (item.value != 50) item.value = 50;
        break;
    case ItemClass::SCROLL_LIGHTNING:
        if (item.value != 150) item.value = 150;
        break;
    case ItemClass::SCROLL_FIREBALL:
        if (item.value != 100) item.value = 100;
        break;
    case ItemClass::SCROLL_CONFUSION:
        if (item.value != 120) item.value = 120;
        break;
    case ItemClass::DAGGER:
        if (item.value != 2) item.value = 2;
        break;
    case ItemClass::SHORT_SWORD:
        if (item.value != 10) item.value = 10;
        break;
    case ItemClass::LONG_SWORD:
        if (item.value != 15) item.value = 15;
        break;
    case ItemClass::STAFF:
        if (item.value != 6) item.value = 6;
        break;
    case ItemClass::LONG_BOW:
        if (item.value != 75) item.value = 75;
        break;
    case ItemClass::LEATHER_ARMOR:
        if (item.value != 5) item.value = 5;
        break;
    case ItemClass::CHAIN_MAIL:
        if (item.value != 75) item.value = 75;
        break;
    case ItemClass::PLATE_MAIL:
        if (item.value != 400) item.value = 400;
        break;
    default:
        // Handle enhanced weapons by checking name pattern
        if (item.actorData.name.find(" +") != std::string::npos)
        {
            // Enhanced weapons - extract enhancement level
            std::string enhancementStr = item.actorData.name.substr(item.actorData.name.find("+") + 1);
            int enhancementLevel = std::stoi(enhancementStr);
            
            // Determine base value from item class
            int baseValue = 0;
            switch (itemClass)
            {
            case ItemClass::DAGGER: baseValue = 2; break;
            case ItemClass::SHORT_SWORD: baseValue = 10; break;
            case ItemClass::LONG_SWORD: baseValue = 15; break;
            case ItemClass::STAFF: baseValue = 6; break;
            case ItemClass::LONG_BOW: baseValue = 75; break;
            default: break;
            }
            
            if (baseValue > 0)
            {
                item.value = calculate_enhanced_value(baseValue, enhancementLevel);
            }
        }
        break;
    }
}
