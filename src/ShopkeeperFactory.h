#pragma once

#include "ShopKeeper.h"

// Forward declarations
class Creature;
struct GameContext;

// Whether a level gets a shopkeeper, and the shop one opens. The shopkeeper itself is
// built from its record in monsters.json like any monster; its Ai calls open_shop.
//
// Example:
//   if (ShopkeeperFactory::should_spawn_shopkeeper(3, ctx)) { ... }
//   ShopkeeperFactory::open_shop(shopkeeper, 3, ctx); // shopkeeper.shop is set and stocked
class ShopkeeperFactory
{
private:
	// Internal helpers
	static ShopType select_shop_type_for_level(
		int dungeonLevel,
		GameContext& ctx);
	static ShopQuality select_shop_quality_for_level(
		int dungeonLevel,
		GameContext& ctx);

public:
	// Spawn probability calculation
	static bool should_spawn_shopkeeper(int dungeonLevel, GameContext& ctx);

	// Gives a shopkeeper a shop of a kind and quality chosen for the level, and stocks it.
	static void open_shop(
		Creature& shopkeeper,
		int dungeonLevel,
		GameContext& ctx);
};
