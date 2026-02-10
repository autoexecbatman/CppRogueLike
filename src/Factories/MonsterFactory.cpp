// file: MonsterFactory.cpp
#include "MonsterFactory.h"
#include "MonsterCreator.h"
#include "../ActorTypes/Monsters.h"
#include "../ActorTypes/Monsters/Spider.h"
#include "../Systems/Shopkeepers/ShopkeeperFactory.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/LevelManager.h"

MonsterFactory::MonsterFactory()
{
    // Registry-driven monsters: adding a new monster only requires a new entry
    // in MonsterCreator's registry — nothing here changes.
    for (const auto& [id, params] : MonsterCreator::get_registry())
    {
        addMonsterType(
        {
            .name = params.name,
            .baseWeight = params.base_weight,
            .levelMinimum = params.level_minimum,
            .levelMaximum = params.level_maximum,
            .levelScaling = params.level_scaling,
            .createFunc = [id](Vector2D pos, GameContext& ctx)
            {
                ctx.creatures->push_back(MonsterCreator::create(pos, id, ctx));
            },
        });
    }

    // Spiders — class-based (unique web-spinning behaviour)
    addMonsterType(
    {
        .name = "Small Spider",
        .baseWeight = 10,
        .levelMinimum = 1,
        .levelMaximum = 0,
        .levelScaling = -0.3f,
        .createFunc = [](Vector2D pos, GameContext& ctx)
        {
            ctx.creatures->push_back(std::make_unique<SmallSpider>(pos, ctx));
        },
    });

    addMonsterType(
    {
        .name = "Giant Spider",
        .baseWeight = 10,
        .levelMinimum = 2,
        .levelMaximum = 0,
        .levelScaling = 0.0f,
        .createFunc = [](Vector2D pos, GameContext& ctx)
        {
            ctx.creatures->push_back(std::make_unique<GiantSpider>(pos, ctx));
        },
    });

    addMonsterType(
    {
        .name = "Web Spinner",
        .baseWeight = 5,
        .levelMinimum = 3,
        .levelMaximum = 0,
        .levelScaling = 0.2f,
        .createFunc = [](Vector2D pos, GameContext& ctx)
        {
            ctx.creatures->push_back(std::make_unique<WebSpinner>(pos, ctx));
        },
    });

    // Mimic — class-based (disguise logic)
    addMonsterType(
    {
        .name = "Mimic",
        .baseWeight = 6,
        .levelMinimum = 2,
        .levelMaximum = 0,
        .levelScaling = 0.5f,
        .createFunc = [](Vector2D pos, GameContext& ctx)
        {
            ctx.creatures->push_back(std::make_unique<Mimic>(pos, ctx));
        },
    });

    // Shopkeeper — class-based (shop inventory logic)
    addMonsterType(
    {
        .name = "Shopkeeper",
        .baseWeight = 20,
        .levelMinimum = 1,
        .levelMaximum = 0,
        .levelScaling = 0.0f,
        .createFunc = [](Vector2D pos, GameContext& ctx)
        {
            const int dungeonLevel = ctx.level_manager->get_dungeon_level();
            if (ShopkeeperFactory::should_spawn_shopkeeper(dungeonLevel, ctx))
            {
                ctx.creatures->push_back(ShopkeeperFactory::create_shopkeeper(pos, dungeonLevel, ctx));
                ctx.message_system->log("Shopkeeper spawned at level " + std::to_string(dungeonLevel));
            }
            else
            {
                ctx.creatures->push_back(MonsterCreator::create(pos, MonsterId::GOBLIN, ctx));
                ctx.message_system->log("Shopkeeper spawn failed, spawned Goblin instead");
            }
        },
    });
}

void MonsterFactory::addMonsterType(const MonsterType& monsterType)
{
    monsterTypes.push_back(monsterType);
}

int MonsterFactory::calculate_weight(const MonsterType& monster, int dungeonLevel) const
{
    auto check_level_requirements = [](int level, int min, int max)
    {
        if (level < min)
            return false;
        if (max > 0 && level > max)
            return false;
        return true;
    };

    if (!check_level_requirements(dungeonLevel, monster.levelMinimum, monster.levelMaximum))
        return 0;

    const float levelFactor = 1.0f + (monster.levelScaling * (dungeonLevel - 1));
    const int weight = static_cast<int>(monster.baseWeight * levelFactor);
    return std::max(1, weight);
}

void MonsterFactory::spawn_random_monster(Vector2D position, int dungeonLevel, GameContext& ctx)
{
    int totalWeight = 0;
    std::vector<int> weights;

    for (const auto& monster : monsterTypes)
    {
        const int weight = calculate_weight(monster, dungeonLevel);
        weights.push_back(weight);
        totalWeight += weight;
    }

    if (totalWeight <= 0)
    {
        ctx.message_system->log("No valid monsters for this dungeon level!");
        return;
    }

    int roll = ctx.dice->roll(1, totalWeight);
    int runningTotal = 0;

    for (size_t i = 0; i < monsterTypes.size(); i++)
    {
        runningTotal += weights[i];
        if (roll <= runningTotal)
        {
            monsterTypes[i].createFunc(position, ctx);
            break;
        }
    }
}

std::vector<std::pair<std::string, float>> MonsterFactory::getCurrentDistribution(int dungeonLevel)
{
    std::vector<std::pair<std::string, float>> distribution;

    int totalWeight = 0;
    std::vector<int> weights;

    for (const auto& monster : monsterTypes)
    {
        const int weight = calculate_weight(monster, dungeonLevel);
        weights.push_back(weight);
        totalWeight += weight;
    }

    for (size_t i = 0; i < monsterTypes.size(); i++)
    {
        if (weights[i] > 0)
        {
            const float percentage = static_cast<float>(weights[i]) / totalWeight * 100.0f;
            distribution.push_back({ monsterTypes[i].name, percentage });
        }
    }

    return distribution;
}
