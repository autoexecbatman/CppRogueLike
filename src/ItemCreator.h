#pragma once

#include <memory>
#include "Actor/Actor.h"
#include "Vector2D.h"

class ItemCreator
{
public:
    // Centralized item creation functions with guaranteed AD&D 2e values
    static std::unique_ptr<Item> create_health_potion(Vector2D pos);
    static std::unique_ptr<Item> create_scroll_lightning(Vector2D pos);
    static std::unique_ptr<Item> create_scroll_fireball(Vector2D pos);
    static std::unique_ptr<Item> create_scroll_confusion(Vector2D pos);
    static std::unique_ptr<Item> create_dagger(Vector2D pos);
    static std::unique_ptr<Item> create_short_sword(Vector2D pos);
    static std::unique_ptr<Item> create_long_sword(Vector2D pos);
    static std::unique_ptr<Item> create_staff(Vector2D pos);
    static std::unique_ptr<Item> create_longbow(Vector2D pos);
    static std::unique_ptr<Item> create_leather_armor(Vector2D pos);
    static std::unique_ptr<Item> create_chain_mail(Vector2D pos);
    static std::unique_ptr<Item> create_plate_mail(Vector2D pos);
    
    // Utility function to fix existing items with incorrect values
    static void ensure_correct_value(Item& item);
};
