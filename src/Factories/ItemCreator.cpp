#include "ItemCreator.h"
#include "../Game.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/Fireball.h"
#include "../ActorTypes/Confuser.h"
#include "../Actor/Pickable.h"
#include "../Items/Armor.h"
#include "../Colors/Colors.h"
#include "../Items/Weapons.h"

std::unique_ptr<Item> ItemCreator::create_health_potion(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'!', "health potion", WHITE_RED_PAIR});
    item->pickable = std::make_unique<Healer>(10);
    item->value = 50; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_scroll_lightning(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'#', "scroll of lightning", WHITE_BLUE_PAIR});
    item->pickable = std::make_unique<LightningBolt>(5, 20);
    item->value = 150; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_scroll_fireball(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'#', "scroll of fireball", RED_YELLOW_PAIR});
    item->pickable = std::make_unique<Fireball>(3, 12);
    item->value = 100; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_scroll_confusion(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'#', "scroll of confusion", WHITE_GREEN_PAIR});
    item->pickable = std::make_unique<Confuser>(10, 8);
    item->value = 120; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_dagger(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "dagger", WHITE_BLACK_PAIR});
    item->pickable = std::make_unique<Dagger>();
    item->value = 2; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_short_sword(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "short sword", WHITE_BLACK_PAIR});
    item->pickable = std::make_unique<ShortSword>();
    item->value = 10; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_long_sword(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "long sword", WHITE_BLACK_PAIR});
    item->pickable = std::make_unique<LongSword>();
    item->value = 15; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_staff(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'/', "staff", WHITE_BLACK_PAIR});
    item->pickable = std::make_unique<Staff>();
    item->value = 6; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_longbow(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{')', "longbow", WHITE_BLUE_PAIR});
    item->pickable = std::make_unique<Longbow>();
    item->value = 75; // AD&D 2e price
    return item;
}

// Enhanced weapon creation functions
std::unique_ptr<Item> ItemCreator::create_enhanced_dagger(Vector2D pos, int enhancementLevel)
{
    auto item = create_dagger(pos);
    if (enhancementLevel > 0)
    {
        // Create enhanced weapon data from base weapon stats
        auto weaponData = std::find_if(game.weapons.begin(), game.weapons.end(),
            [](const Weapons& w) { return w.name == "Dagger"; });
        
        if (weaponData != game.weapons.end())
        {
            Weapons enhancedWeapon = *weaponData;
            enhancedWeapon.setEnhancementLevel(enhancementLevel);
            
            item->actorData.name = enhancedWeapon.getDisplayName();
            item->value = calculate_enhanced_value(2, enhancementLevel);
            
            // Enhanced weapons get visual distinction
            item->actorData.color = WHITE_GREEN_PAIR;
        }
    }
    return item;
}

std::unique_ptr<Item> ItemCreator::create_enhanced_short_sword(Vector2D pos, int enhancementLevel)
{
    auto item = create_short_sword(pos);
    if (enhancementLevel > 0)
    {
        auto weaponData = std::find_if(game.weapons.begin(), game.weapons.end(),
            [](const Weapons& w) { return w.name == "Short Sword"; });
        
        if (weaponData != game.weapons.end())
        {
            Weapons enhancedWeapon = *weaponData;
            enhancedWeapon.setEnhancementLevel(enhancementLevel);
            
            item->actorData.name = enhancedWeapon.getDisplayName();
            item->value = calculate_enhanced_value(10, enhancementLevel);
            item->actorData.color = WHITE_GREEN_PAIR;
        }
    }
    return item;
}

std::unique_ptr<Item> ItemCreator::create_enhanced_long_sword(Vector2D pos, int enhancementLevel)
{
    auto item = create_long_sword(pos);
    if (enhancementLevel > 0)
    {
        auto weaponData = std::find_if(game.weapons.begin(), game.weapons.end(),
            [](const Weapons& w) { return w.name == "Long Sword"; });
        
        if (weaponData != game.weapons.end())
        {
            Weapons enhancedWeapon = *weaponData;
            enhancedWeapon.setEnhancementLevel(enhancementLevel);
            
            item->actorData.name = enhancedWeapon.getDisplayName();
            item->value = calculate_enhanced_value(15, enhancementLevel);
            item->actorData.color = WHITE_GREEN_PAIR;
        }
    }
    return item;
}

std::unique_ptr<Item> ItemCreator::create_enhanced_staff(Vector2D pos, int enhancementLevel)
{
    auto item = create_staff(pos);
    if (enhancementLevel > 0)
    {
        auto weaponData = std::find_if(game.weapons.begin(), game.weapons.end(),
            [](const Weapons& w) { return w.name == "Staff"; });
        
        if (weaponData != game.weapons.end())
        {
            Weapons enhancedWeapon = *weaponData;
            enhancedWeapon.setEnhancementLevel(enhancementLevel);
            
            item->actorData.name = enhancedWeapon.getDisplayName();
            item->value = calculate_enhanced_value(6, enhancementLevel);
            item->actorData.color = WHITE_GREEN_PAIR;
        }
    }
    return item;
}

std::unique_ptr<Item> ItemCreator::create_enhanced_longbow(Vector2D pos, int enhancementLevel)
{
    auto item = create_longbow(pos);
    if (enhancementLevel > 0)
    {
        auto weaponData = std::find_if(game.weapons.begin(), game.weapons.end(),
            [](const Weapons& w) { return w.name == "Longbow"; });
        
        if (weaponData != game.weapons.end())
        {
            Weapons enhancedWeapon = *weaponData;
            enhancedWeapon.setEnhancementLevel(enhancementLevel);
            
            item->actorData.name = enhancedWeapon.getDisplayName();
            item->value = calculate_enhanced_value(75, enhancementLevel);
            item->actorData.color = WHITE_GREEN_PAIR;
        }
    }
    return item;
}

// Random enhancement chance functions
std::unique_ptr<Item> ItemCreator::create_random_weapon(Vector2D pos, int dungeonLevel)
{
    // Random weapon type selection
    int weaponType = game.d.roll(1, 5);
    
    switch (weaponType)
    {
        case 1: return create_weapon_with_enhancement_chance(pos, dungeonLevel); // Dagger
        case 2: return create_enhanced_short_sword(pos, determine_enhancement_level(dungeonLevel));
        case 3: return create_enhanced_long_sword(pos, determine_enhancement_level(dungeonLevel));
        case 4: return create_enhanced_staff(pos, determine_enhancement_level(dungeonLevel));
        case 5: return create_enhanced_longbow(pos, determine_enhancement_level(dungeonLevel));
        default: return create_enhanced_dagger(pos, determine_enhancement_level(dungeonLevel));
    }
}

std::unique_ptr<Item> ItemCreator::create_weapon_with_enhancement_chance(Vector2D pos, int dungeonLevel)
{
    int enhancementChance = calculate_enhancement_chance(dungeonLevel);
    int roll = game.d.roll(1, 100);
    
    if (roll <= enhancementChance)
    {
        int enhancementLevel = determine_enhancement_level(dungeonLevel);
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

int ItemCreator::determine_enhancement_level(int dungeonLevel)
{
    int roll = game.d.roll(1, 100);
    
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
    item->value = 5; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_chain_mail(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'[', "chain mail", BROWN_BLACK_PAIR});
    item->pickable = std::make_unique<ChainMail>();
    item->value = 75; // AD&D 2e price
    return item;
}

std::unique_ptr<Item> ItemCreator::create_plate_mail(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'[', "plate mail", BROWN_BLACK_PAIR});
    item->pickable = std::make_unique<PlateMail>();
    item->value = 400; // AD&D 2e price
    return item;
}

void ItemCreator::ensure_correct_value(Item& item)
{
    // Fix items with incorrect values based on their name and pickable type
    if (item.actorData.name == "health potion" && item.value != 50)
    {
        item.value = 50;
    }
    else if (item.actorData.name == "scroll of lightning" && item.value != 150)
    {
        item.value = 150;
    }
    else if (item.actorData.name == "scroll of fireball" && item.value != 100)
    {
        item.value = 100;
    }
    else if (item.actorData.name == "scroll of confusion" && item.value != 120)
    {
        item.value = 120;
    }
    else if (item.actorData.name == "dagger" && item.value != 2)
    {
        item.value = 2;
    }
    else if (item.actorData.name == "short sword" && item.value != 10)
    {
        item.value = 10;
    }
    else if (item.actorData.name == "long sword" && item.value != 15)
    {
        item.value = 15;
    }
    else if (item.actorData.name == "staff" && item.value != 6)
    {
        item.value = 6;
    }
    else if (item.actorData.name == "longbow" && item.value != 75)
    {
        item.value = 75;
    }
    else if (item.actorData.name == "leather armor" && item.value != 5)
    {
        item.value = 5;
    }
    else if (item.actorData.name == "chain mail" && item.value != 75)
    {
        item.value = 75;
    }
    else if (item.actorData.name == "plate mail" && item.value != 400)
    {
        item.value = 400;
    }
    // Handle enhanced weapons
    else if (item.actorData.name.find(" +") != std::string::npos)
    {
        // Enhanced weapons - extract base name and enhancement level
        std::string baseName = item.actorData.name.substr(0, item.actorData.name.find(" +"));
        std::string enhancementStr = item.actorData.name.substr(item.actorData.name.find("+") + 1);
        int enhancementLevel = std::stoi(enhancementStr);
        
        // Determine base value and calculate enhanced value
        int baseValue = 0;
        if (baseName == "dagger") baseValue = 2;
        else if (baseName == "short sword") baseValue = 10;
        else if (baseName == "long sword") baseValue = 15;
        else if (baseName == "staff") baseValue = 6;
        else if (baseName == "longbow") baseValue = 75;
        
        if (baseValue > 0)
        {
            item.value = calculate_enhanced_value(baseValue, enhancementLevel);
        }
    }
}
