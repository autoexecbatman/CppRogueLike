#include "ItemCreator.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/Fireball.h"
#include "../ActorTypes/Confuser.h"
#include "../ActorTypes/Teleporter.h"
#include "../Actor/Pickable.h"
#include "../Items/Armor.h"
#include "../Items/Jewelry.h"
#include "../Items/MagicalItemEffects.h"
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

// Authentic AD&D 2e Magical Helms - provide special abilities, NOT AC bonuses
std::unique_ptr<Item> ItemCreator::create_helm_of_brilliance(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'^', "helm of brilliance", YELLOW_BLACK_PAIR});
    item->pickable = std::make_unique<MagicalHelm>(MagicalEffect::BRILLIANCE);
    item->itemClass = ItemClass::HELMET;
    item->set_value(12000); // Very rare and powerful
    return item;
}

std::unique_ptr<Item> ItemCreator::create_helm_of_teleportation(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'^', "helm of teleportation", MAGENTA_BLACK_PAIR});
    item->pickable = std::make_unique<MagicalHelm>(MagicalEffect::TELEPORTATION);
    item->itemClass = ItemClass::HELMET;
    item->set_value(7500);
    return item;
}

// Authentic AD&D 2e Magical Rings
std::unique_ptr<Item> ItemCreator::create_ring_of_protection_plus_1(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'=', "ring of protection +1", CYAN_BLACK_PAIR});
    item->pickable = std::make_unique<MagicalRing>(MagicalEffect::PROTECTION_PLUS_1);
    item->itemClass = ItemClass::RING;
    item->set_value(2000);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_ring_of_protection_plus_2(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'=', "ring of protection +2", CYAN_BLACK_PAIR});
    item->pickable = std::make_unique<MagicalRing>(MagicalEffect::PROTECTION_PLUS_2);
    item->itemClass = ItemClass::RING;
    item->set_value(5000);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_ring_of_free_action(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'=', "ring of free action", GREEN_BLACK_PAIR});
    item->pickable = std::make_unique<MagicalRing>(MagicalEffect::FREE_ACTION);
    item->itemClass = ItemClass::RING;
    item->set_value(4000);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_ring_of_regeneration(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'=', "ring of regeneration", RED_BLACK_PAIR});
    item->pickable = std::make_unique<MagicalRing>(MagicalEffect::REGENERATION);
    item->itemClass = ItemClass::RING;
    item->set_value(8000);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_ring_of_invisibility(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'=', "ring of invisibility", WHITE_BLACK_PAIR});
    item->pickable = std::make_unique<MagicalRing>(MagicalEffect::INVISIBILITY);
    item->itemClass = ItemClass::RING;
    item->set_value(6000);
    return item;
}

// Amulet creation functions (simplified - not authentic AD&D 2e)
std::unique_ptr<Item> ItemCreator::create_amulet_of_health(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'"', "amulet of health", RED_BLACK_PAIR});
    auto amulet = std::make_unique<JewelryAmulet>();
    amulet->con_bonus = 1;
    item->pickable = std::move(amulet);
    item->itemClass = ItemClass::AMULET;
    item->set_value(200);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_amulet_of_wisdom(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'"', "amulet of wisdom", BLUE_BLACK_PAIR});
    auto amulet = std::make_unique<JewelryAmulet>();
    amulet->wis_bonus = 1;
    item->pickable = std::move(amulet);
    item->itemClass = ItemClass::AMULET;
    item->set_value(200);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_amulet_of_protection(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'"', "amulet of protection", CYAN_BLACK_PAIR});
    auto amulet = std::make_unique<JewelryAmulet>();
    item->pickable = std::move(amulet);
    item->itemClass = ItemClass::AMULET;
    item->set_value(150);
    return item;
}

// Authentic AD&D 2e Gauntlets - stat bonuses from gauntlets, not rings
std::unique_ptr<Item> ItemCreator::create_gauntlets_of_ogre_power(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'[', "gauntlets of ogre power", RED_BLACK_PAIR});
    auto gauntlets = std::make_unique<Gauntlets>();
    gauntlets->str_bonus = 18;
    gauntlets->is_set_mode = true;  // Sets STR to 18/00, not adds
    item->pickable = std::move(gauntlets);
    item->itemClass = ItemClass::GAUNTLETS;
    item->set_value(3000);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_gauntlets_of_dexterity(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'[', "gauntlets of dexterity", GREEN_BLACK_PAIR});
    auto gauntlets = std::make_unique<Gauntlets>();
    gauntlets->dex_bonus = 2;  // +2 DEX (adds, not sets)
    item->pickable = std::move(gauntlets);
    item->itemClass = ItemClass::GAUNTLETS;
    item->set_value(2000);
    return item;
}

// Authentic AD&D 2e Girdles - giant strength
std::unique_ptr<Item> ItemCreator::create_girdle_of_hill_giant_strength(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'[', "girdle of hill giant", BROWN_BLACK_PAIR});
    auto girdle = std::make_unique<Girdle>();
    girdle->str_bonus = 19;
    girdle->is_set_mode = true;  // Sets STR to 19, not adds
    item->pickable = std::move(girdle);
    item->itemClass = ItemClass::GIRDLE;
    item->set_value(5000);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_girdle_of_frost_giant_strength(Vector2D pos)
{
    auto item = std::make_unique<Item>(pos, ActorData{'[', "girdle of frost giant", CYAN_BLACK_PAIR});
    auto girdle = std::make_unique<Girdle>();
    girdle->str_bonus = 21;
    girdle->is_set_mode = true;  // Sets STR to 21, not adds
    item->pickable = std::move(girdle);
    item->itemClass = ItemClass::GIRDLE;
    item->set_value(8000);
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
