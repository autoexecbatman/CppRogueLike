#pragma once

#include "../../Utils/Vector2D.h"
#include "../ShopKeeper.h"
#include <memory>

// Forward declarations
class Creature;

/**
 * Single source of truth for all shopkeeper creation and management
 * Consolidates: spawn logic, shop data, AI assignment, and inventory generation
 */
class ShopkeeperFactory
{
public:
    // Main creation method - single source for shopkeeper spawning
    static std::unique_ptr<Creature> create_shopkeeper(Vector2D position, int dungeonLevel);
    
    // Spawn probability calculation
    static bool should_spawn_shopkeeper(int dungeonLevel);
    
    // Configure shopkeeper stats, AI, and shop component
    static void configure_shopkeeper(Creature& shopkeeper, int dungeonLevel);
    
private:
    // Internal helpers
    static ShopType select_shop_type_for_level(int dungeonLevel);
    static ShopQuality select_shop_quality_for_level(int dungeonLevel);
};
