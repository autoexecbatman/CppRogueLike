#pragma once

#include <memory>

#include "../Actor/Actor.h"

struct Vector2D;
struct GameContext;
enum class ItemClass;

class ItemCreator
{
public:
    // Centralized item creation functions with guaranteed AD&D 2e values
    static std::unique_ptr<Item> create_scroll_teleportation(Vector2D pos);
    static std::unique_ptr<Item> create_dagger(Vector2D pos);
    static std::unique_ptr<Item> create_short_sword(Vector2D pos);
    static std::unique_ptr<Item> create_long_sword(Vector2D pos);
    static std::unique_ptr<Item> create_staff(Vector2D pos);
    static std::unique_ptr<Item> create_longbow(Vector2D pos);
    static std::unique_ptr<Item> create_greatsword(Vector2D pos);
    static std::unique_ptr<Item> create_battle_axe(Vector2D pos);
    static std::unique_ptr<Item> create_great_axe(Vector2D pos);
    static std::unique_ptr<Item> create_war_hammer(Vector2D pos);
    static std::unique_ptr<Item> create_shield(Vector2D pos);
    static std::unique_ptr<Item> create_leather_armor(Vector2D pos);
    static std::unique_ptr<Item> create_chain_mail(Vector2D pos);
    static std::unique_ptr<Item> create_plate_mail(Vector2D pos);
    static std::unique_ptr<Item> create_gold_pile(Vector2D pos, GameContext& ctx);

    // Amulet creation functions
    static std::unique_ptr<Item> create_amulet_of_health(Vector2D pos);
    static std::unique_ptr<Item> create_amulet_of_wisdom(Vector2D pos);
    static std::unique_ptr<Item> create_amulet_of_protection(Vector2D pos);

    // Authentic AD&D 2e magical helms
    static std::unique_ptr<Item> create_helm_of_brilliance(Vector2D pos);
    static std::unique_ptr<Item> create_helm_of_teleportation(Vector2D pos);

    // Authentic AD&D 2e magical rings
    static std::unique_ptr<Item> create_ring_of_protection_plus_1(Vector2D pos);
    static std::unique_ptr<Item> create_ring_of_protection_plus_2(Vector2D pos);
    static std::unique_ptr<Item> create_ring_of_free_action(Vector2D pos);
    static std::unique_ptr<Item> create_ring_of_regeneration(Vector2D pos);
    static std::unique_ptr<Item> create_ring_of_invisibility(Vector2D pos);

    // Authentic AD&D 2e gauntlets (stat bonuses)
    static std::unique_ptr<Item> create_gauntlets_of_ogre_power(Vector2D pos);
    static std::unique_ptr<Item> create_gauntlets_of_dexterity(Vector2D pos);

    // Authentic AD&D 2e girdles (stat bonuses)
    static std::unique_ptr<Item> create_girdle_of_hill_giant_strength(Vector2D pos);
    static std::unique_ptr<Item> create_girdle_of_frost_giant_strength(Vector2D pos);

    // Food creation functions
    static std::unique_ptr<Item> create_ration(Vector2D pos);
    static std::unique_ptr<Item> create_fruit(Vector2D pos);
    static std::unique_ptr<Item> create_bread(Vector2D pos);
    static std::unique_ptr<Item> create_meat(Vector2D pos);
    
    // Artifact creation functions
    static std::unique_ptr<Item> create_amulet_of_yendor(Vector2D pos);
    
    // Enhanced weapon creation functions
    static std::unique_ptr<Item> create_enhanced_dagger(Vector2D pos, int enhancementLevel);
    static std::unique_ptr<Item> create_enhanced_short_sword(Vector2D pos, int enhancementLevel);
    static std::unique_ptr<Item> create_enhanced_long_sword(Vector2D pos, int enhancementLevel);
    static std::unique_ptr<Item> create_enhanced_staff(Vector2D pos, int enhancementLevel);
    static std::unique_ptr<Item> create_enhanced_longbow(Vector2D pos, int enhancementLevel);
    
    // SINGLE SOURCE OF TRUTH - All item creation hardcoded here
    static std::unique_ptr<Item> create_health_potion(Vector2D pos);
    static std::unique_ptr<Item> create_scroll_lightning(Vector2D pos);
    static std::unique_ptr<Item> create_scroll_fireball(Vector2D pos);
    static std::unique_ptr<Item> create_scroll_confusion(Vector2D pos);
    static std::unique_ptr<Item> create_invisibility_potion(Vector2D pos);
    
    // Weapons - UNIFIED creation
    static std::unique_ptr<Item> create_iron_sword(Vector2D pos);
    static std::unique_ptr<Item> create_steel_sword(Vector2D pos);
    static std::unique_ptr<Item> create_mace(Vector2D pos);
    
    // Armor - UNIFIED creation  
    static std::unique_ptr<Item> create_studded_leather(Vector2D pos);
    static std::unique_ptr<Item> create_scale_mail(Vector2D pos);
    
    // Random generation with single source
    static std::unique_ptr<Item> create_random_weapon(Vector2D pos, GameContext& ctx, int dungeonLevel = 1);
    static std::unique_ptr<Item> create_random_armor(Vector2D pos, int dungeonLevel = 1);
    static std::unique_ptr<Item> create_random_potion(Vector2D pos, int dungeonLevel = 1);
    static std::unique_ptr<Item> create_random_scroll(Vector2D pos, GameContext& ctx, int dungeonLevel = 1);
    static std::unique_ptr<Item> create_weapon_with_enhancement_chance(Vector2D pos, GameContext& ctx, int dungeonLevel = 1);

    // Enhancement utility functions
    static int calculate_enhancement_chance(int dungeonLevel);
    static int determine_enhancement_level(GameContext& ctx, int dungeonLevel);
    static int calculate_enhanced_value(int baseValue, int enhancementLevel);

    // Centralized weapon creation by ItemClass (proper approach)
    static std::unique_ptr<Item> create_weapon_by_class(Vector2D pos, ItemClass weaponClass);
    static void create_pickable_from_itemclass(Item* item, ItemClass itemClass);
};
