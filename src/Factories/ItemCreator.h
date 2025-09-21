#pragma once

#include <memory>
#include "../Actor/Actor.h"
#include "../Utils/Vector2D.h"

class ItemCreator
{
public:
    // Centralized item creation functions with guaranteed AD&D 2e values
    static std::unique_ptr<Item> create_health_potion(Vector2D pos);
    static std::unique_ptr<Item> create_scroll_lightning(Vector2D pos);
    static std::unique_ptr<Item> create_scroll_fireball(Vector2D pos);
    static std::unique_ptr<Item> create_scroll_confusion(Vector2D pos);
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
    static std::unique_ptr<Item> create_gold_pile(Vector2D pos);
    
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
    
    // Random enhancement chance functions
    static std::unique_ptr<Item> create_random_weapon(Vector2D pos, int dungeonLevel = 1);
    static std::unique_ptr<Item> create_weapon_with_enhancement_chance(Vector2D pos, int dungeonLevel = 1);
    
    // Enhancement utility functions
    static int calculate_enhancement_chance(int dungeonLevel);
    static int determine_enhancement_level(int dungeonLevel);
    static int calculate_enhanced_value(int baseValue, int enhancementLevel);
    
    // Utility function to fix existing items with incorrect values
    static void ensure_correct_value(Item& item);
};
