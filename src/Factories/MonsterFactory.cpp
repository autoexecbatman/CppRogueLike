// file: MonsterFactory.cpp
#include "MonsterFactory.h"
#include "../ActorTypes/Monsters.h"
#include "../ActorTypes/Monsters/Spider.h"
#include "../Random/RandomDice.h"
#include "../Ai/AiMonsterRanged.h"
#include "../Systems/Shopkeepers/ShopkeeperFactory.h"
#include "ItemCreator.h"
#include "../Items/Food.h"
#include "../Actor/InventoryOperations.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/LevelManager.h"
#include "../Map/Map.h"

using namespace InventoryOperations; // For clean function calls

MonsterFactory::MonsterFactory()
{
    // Initialize with all monster types
    addMonsterType({
        "Mimic", 6, 2, 0, 0.5f,
        [](Vector2D pos, GameContext& ctx) { ctx.creatures->push_back(std::make_unique<Mimic>(pos, ctx)); }
        });

    addMonsterType({
        "Small Spider", 10, 1, 0, -0.3f,
        [](Vector2D pos, GameContext& ctx) { ctx.creatures->push_back(std::make_unique<SmallSpider>(pos, ctx)); }
        });

    addMonsterType({
        "Giant Spider", 10, 2, 0, 0.0f,
        [](Vector2D pos, GameContext& ctx) { ctx.creatures->push_back(std::make_unique<GiantSpider>(pos, ctx)); }
        });

    addMonsterType({
        "Web Spinner", 5, 3, 0, 0.2f,
        [](Vector2D pos, GameContext& ctx) { ctx.creatures->push_back(std::make_unique<WebSpinner>(pos, ctx)); }
        });

    addMonsterType({
        "Goblin", 30, 1, 5, -0.2f,
        [](Vector2D pos, GameContext& ctx) { ctx.creatures->push_back(std::make_unique<Goblin>(pos, ctx)); }
        });

    addMonsterType({
        "Orc", 15, 2, 0, 0.0f,
        [](Vector2D pos, GameContext& ctx) { ctx.creatures->push_back(std::make_unique<Orc>(pos, ctx)); }
        });

    addMonsterType({
        "Archer", 10, 2, 0, 0.2f,
        [](Vector2D pos, GameContext& ctx) { ctx.creatures->push_back(std::make_unique<Archer>(pos, ctx)); }
        });

    addMonsterType({
        "Mage", 10, 3, 0, 0.3f,
        [](Vector2D pos, GameContext& ctx) { ctx.creatures->push_back(std::make_unique<Mage>(pos, ctx)); }
        });

    addMonsterType({
        "Troll", 7, 4, 0, 0.5f,
        [](Vector2D pos, GameContext& ctx) { ctx.creatures->push_back(std::make_unique<Troll>(pos, ctx)); }
        });

    addMonsterType({
        "Dragon", 3, 5, 0, 1.0f,
        [](Vector2D pos, GameContext& ctx) { ctx.creatures->push_back(std::make_unique<Dragon>(pos, ctx)); }
        });

    // Add shopkeeper to the spawn table with proper probability management
    addMonsterType({
        "Shopkeeper", 20, 1, 0, 0.0f,  // INCREASED weight from 8 to 20 for more frequent spawning
        [](Vector2D pos, GameContext& ctx) {
            int dungeonLevel = ctx.level_manager->get_dungeon_level();
            // Use ShopkeeperFactory's probability system for actual spawning
            if (ShopkeeperFactory::should_spawn_shopkeeper(dungeonLevel, ctx))
            {
                auto shopkeeper = ShopkeeperFactory::create_shopkeeper(pos, dungeonLevel, ctx);
                ctx.creatures->push_back(std::move(shopkeeper));
                ctx.message_system->log("Shopkeeper spawned via MonsterFactory with proper probability at level " + std::to_string(dungeonLevel));
            }
            else
            {
                // If shopkeeper spawn probability fails, spawn alternative monster
                ctx.creatures->push_back(std::make_unique<Goblin>(pos, ctx));
                ctx.message_system->log("Shopkeeper spawn failed, spawned Goblin instead");
            }
        }
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
            if (level < min) // monster can't appear below min level
            {
                return false;
            }
            else if (max > 0 && level > max) // monster can't appear above max level and has a max level
            {
                return false;
            }
            else
            {
                return true;
            }
        };

    // Check level requirements
    if (!check_level_requirements(dungeonLevel, monster.levelMinimum, monster.levelMaximum))
    {
        return 0; // Monster not available at this level
    }

    // Calculate scaled weight based on dungeon level
    float levelFactor = 1.0f + (monster.levelScaling * (dungeonLevel - 1));
    int weight = static_cast<int>(monster.baseWeight * levelFactor);

    // Ensure weight is at least 1 if monster is available at this level
    return std::max(1, weight);
}

void MonsterFactory::spawn_random_monster(Vector2D position, int dungeonLevel, GameContext& ctx)
{
    // Calculate total weights for current dungeon level
    int totalWeight = 0;
    std::vector<int> weights;

    for (const auto& monster : monsterTypes) {
        int weight = calculate_weight(monster, dungeonLevel);
        weights.push_back(weight);
        totalWeight += weight;
    }

    // If no valid monsters for this level, do nothing
    if (totalWeight <= 0) {
        ctx.message_system->log("No valid monsters for this dungeon level!");
        return;
    }

    // Roll random number and select monster
    int roll = ctx.dice->roll(1, totalWeight);
    int runningTotal = 0;

    for (size_t i = 0; i < monsterTypes.size(); i++) {
        runningTotal += weights[i];
        if (roll <= runningTotal) {
            // Create the monster
            monsterTypes[i].createFunc(position, ctx);

            // Check if special AI needed for ranged attackers
            Creature* monster = ctx.map->get_actor(position, ctx);
            if (monster && monster->has_state(ActorState::IS_RANGED)) {
                // Replace standard AI with ranged AI
                monster->ai = std::make_unique<AiMonsterRanged>();
            }

            break;
        }
    }
}

std::vector<std::pair<std::string, float>> MonsterFactory::getCurrentDistribution(int dungeonLevel) {
    std::vector<std::pair<std::string, float>> distribution;

    // Calculate total weights for current dungeon level
    int totalWeight = 0;
    std::vector<int> weights;

    for (const auto& monster : monsterTypes) {
        int weight = calculate_weight(monster, dungeonLevel);
        weights.push_back(weight);
        totalWeight += weight;
    }

    // Calculate percentage for each monster
    for (size_t i = 0; i < monsterTypes.size(); i++) {
        if (weights[i] > 0) {
            float percentage = static_cast<float>(weights[i]) / totalWeight * 100.0f;
            distribution.push_back({ monsterTypes[i].name, percentage });
        }
    }

    return distribution;
}