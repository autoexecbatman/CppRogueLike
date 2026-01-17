#include "ShopkeeperFactory.h"
#include "../../Actor/Actor.h"
#include "../../Ai/AiShopkeeper.h"
#include "../../Colors/Colors.h"
#include "../../Actor/Attacker.h"
#include "../../Actor/Destructible.h"
#include "../../Random/RandomDice.h"
#include "../../Core/GameContext.h"
#include "../../Systems/MessageSystem.h"

std::unique_ptr<Creature> ShopkeeperFactory::create_shopkeeper(Vector2D position, int dungeonLevel, GameContext& ctx)
{
    // Create base creature
    auto shopkeeper = std::make_unique<Creature>(position, ActorData{ 'S', "Shopkeeper", YELLOW_BLACK_PAIR });

    // Configure all shopkeeper components in one place
    configure_shopkeeper(*shopkeeper, dungeonLevel, ctx);

    return shopkeeper;
}

bool ShopkeeperFactory::should_spawn_shopkeeper(int dungeonLevel, GameContext& ctx)
{
    // Base 8% chance, increases by 2% per level, cap at 20%
    int shopkeeperChance = 8 + (dungeonLevel * 2);
    shopkeeperChance = std::min(shopkeeperChance, 20);

    return ctx.dice->d100() <= shopkeeperChance;
}

void ShopkeeperFactory::configure_shopkeeper(Creature& shopkeeper, int dungeonLevel, GameContext& ctx)
{
    // Set AI - single source for shopkeeper behavior
    shopkeeper.ai = std::make_unique<AiShopkeeper>();

    // Set combat stats - non-hostile defensive stats (use MonsterDestructible for proper save/load)
    shopkeeper.destructible = std::make_unique<MonsterDestructible>(100, 20, "the shopkeeper's corpse", 0, 20, 10);
    shopkeeper.attacker = std::make_unique<Attacker>(DamageValues::Dagger());
    shopkeeper.set_weapon_equipped("Dagger");

    // Create shop component with level-appropriate configuration
    ShopType shopType = select_shop_type_for_level(dungeonLevel, ctx);
    ShopQuality shopQuality = select_shop_quality_for_level(dungeonLevel, ctx);
    shopkeeper.shop = std::make_unique<ShopKeeper>(shopType, shopQuality);

    ctx.message_system->log("Created shopkeeper: " + shopkeeper.shop->shop_name + " (Level " + std::to_string(dungeonLevel) + ")");
}

ShopType ShopkeeperFactory::select_shop_type_for_level(int dungeonLevel, GameContext& ctx)
{
    // Early levels favor general stores, deeper levels get specialized shops
    if (dungeonLevel <= 2)
    {
        return (ctx.dice->d100() <= 60) ? ShopType::GENERAL_STORE : ShopType::WEAPON_SHOP;
    }
    else if (dungeonLevel <= 4)
    {
        int roll = ctx.dice->d100();
        if (roll <= 25) return ShopType::WEAPON_SHOP;
        if (roll <= 50) return ShopType::ARMOR_SHOP;
        if (roll <= 75) return ShopType::POTION_SHOP;
        return ShopType::GENERAL_STORE;
    }
    else
    {
        // Higher levels get full variety including scroll shops
        int roll = ctx.dice->d100();
        if (roll <= 20) return ShopType::WEAPON_SHOP;
        if (roll <= 40) return ShopType::ARMOR_SHOP;
        if (roll <= 60) return ShopType::POTION_SHOP;
        if (roll <= 80) return ShopType::SCROLL_SHOP;
        return ShopType::ADVENTURING_GEAR;
    }
}

ShopQuality ShopkeeperFactory::select_shop_quality_for_level(int dungeonLevel, GameContext& ctx)
{
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
