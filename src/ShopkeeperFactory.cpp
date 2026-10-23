#include <algorithm>
#include <cassert>
#include <format>
#include <memory>

#include "Creature.h"
#include "GameContext.h"
#include "MessageSystem.h"
#include "RandomDice.h"
#include "ShopKeeper.h"
#include "ShopkeeperFactory.h"

bool ShopkeeperFactory::should_spawn_shopkeeper(
	int dungeonLevel,
	GameContext& ctx)
{
	assert(
		ctx.dice &&
		"ShopkeeperFactory::should_spawn_shopkeeper: called without dice");

	// Base 8% chance, increases by 2% per level, cap at 20%
	int shopkeeperChance = 8 + (dungeonLevel * 2);
	shopkeeperChance = std::min(shopkeeperChance, 20);

	return ctx.dice->d100() <= shopkeeperChance;
}

void ShopkeeperFactory::open_shop(
	Creature& shopkeeper,
	int dungeonLevel,
	GameContext& ctx)
{
	assert(ctx.dice && "ShopkeeperFactory::open_shop: called without dice");

	// Create shop component with level-appropriate configuration
	ShopType shopType = select_shop_type_for_level(dungeonLevel, ctx);
	ShopQuality shopQuality = select_shop_quality_for_level(dungeonLevel, ctx);
	shopkeeper.shop =
		std::make_unique<ShopKeeper>(shopType, shopQuality, *ctx.dice);
	shopkeeper.shop->generate_initial_inventory(dungeonLevel, ctx);

	ctx.messageSystem->log(
		std::format(
			"Created shopkeeper: {} (Level {})",
			shopkeeper.shop->get_shop_name(),
			dungeonLevel));
}

ShopType ShopkeeperFactory::select_shop_type_for_level(
	int dungeonLevel,
	GameContext& ctx)
{
	assert(
		ctx.dice &&
		"ShopkeeperFactory::select_shop_type_for_level: called without dice");

	// Early levels favor general stores, deeper levels get specialized shops
	if (dungeonLevel <= 2)
	{
		if (ctx.dice->d100() <= 60)
		{
			return ShopType::GENERAL_STORE;
		}
		return ShopType::WEAPON_SHOP;
	}
	else if (dungeonLevel <= 4)
	{
		int roll = ctx.dice->d100();
		if (roll <= 25)
		{
			return ShopType::WEAPON_SHOP;
		}
		if (roll <= 50)
		{
			return ShopType::ARMOR_SHOP;
		}
		if (roll <= 75)
		{
			return ShopType::POTION_SHOP;
		}
		return ShopType::GENERAL_STORE;
	}
	else
	{
		// Higher levels get full variety including scroll shops
		int roll = ctx.dice->d100();
		if (roll <= 20)
		{
			return ShopType::WEAPON_SHOP;
		}
		if (roll <= 40)
		{
			return ShopType::ARMOR_SHOP;
		}
		if (roll <= 60)
		{
			return ShopType::POTION_SHOP;
		}
		if (roll <= 80)
		{
			return ShopType::SCROLL_SHOP;
		}
		return ShopType::ADVENTURING_GEAR;
	}
}

ShopQuality ShopkeeperFactory::select_shop_quality_for_level(
	int dungeonLevel,
	GameContext& ctx)
{
	assert(
		ctx.dice &&
		"ShopkeeperFactory::select_shop_quality_for_level: called without dice");

	// Quality improves with dungeon depth
	int qualityRoll = ctx.dice->d100();
	int levelBonus = dungeonLevel * 5; // 5% quality improvement per level

	if (qualityRoll + levelBonus >= 85)
	{
		return ShopQuality::EXCELLENT;
	}
	else if (qualityRoll + levelBonus >= 60)
	{
		return ShopQuality::GOOD;
	}
	else if (qualityRoll + levelBonus >= 30)
	{
		return ShopQuality::AVERAGE;
	}
	else
	{
		return ShopQuality::POOR;
	}
}
