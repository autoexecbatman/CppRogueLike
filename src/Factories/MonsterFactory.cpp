// file: MonsterFactory.cpp
#include "MonsterFactory.h"
#include "../Game.h"
#include "../ActorTypes/Monsters.h"
#include "../ActorTypes/Monsters/Spider.h"
#include "../Random/RandomDice.h"
#include "../Ai/AiMonsterRanged.h"
#include "../Systems/Shopkeepers/ShopkeeperFactory.h"
#include "ItemCreator.h"
#include "../Items/Food.h"
#include "../Actor/InventoryOperations.h"

using namespace InventoryOperations; // For clean function calls

MonsterFactory::MonsterFactory()
{
    // Initialize with all monster types
    addMonsterType({
        "Mimic", 6, 2, 0, 0.5f,
        [](Vector2D pos) { game.creatures.push_back(std::make_unique<Mimic>(pos)); }
        });

    addMonsterType({
        "Small Spider", 10, 1, 0, -0.3f,
        [](Vector2D pos) { game.creatures.push_back(std::make_unique<SmallSpider>(pos)); }
        });

    addMonsterType({
        "Giant Spider", 10, 2, 0, 0.0f,
        [](Vector2D pos) { game.creatures.push_back(std::make_unique<GiantSpider>(pos)); }
        });

    addMonsterType({
        "Web Spinner", 5, 3, 0, 0.2f,
        [](Vector2D pos) { game.creatures.push_back(std::make_unique<WebSpinner>(pos)); }
        });

    addMonsterType({
        "Goblin", 30, 1, 5, -0.2f,
        [](Vector2D pos) { game.creatures.push_back(std::make_unique<Goblin>(pos)); }
        });

    addMonsterType({
        "Orc", 15, 2, 0, 0.0f,
        [](Vector2D pos) { game.creatures.push_back(std::make_unique<Orc>(pos)); }
        });

    addMonsterType({
        "Archer", 10, 2, 0, 0.2f,
        [](Vector2D pos) { game.creatures.push_back(std::make_unique<Archer>(pos)); }
        });

    addMonsterType({
        "Mage", 10, 3, 0, 0.3f,
        [](Vector2D pos) { game.creatures.push_back(std::make_unique<Mage>(pos)); }
        });

    addMonsterType({
        "Troll", 7, 4, 0, 0.5f,
        [](Vector2D pos) { game.creatures.push_back(std::make_unique<Troll>(pos)); }
        });

    addMonsterType({
        "Dragon", 3, 5, 0, 1.0f,
        [](Vector2D pos) { game.creatures.push_back(std::make_unique<Dragon>(pos)); }
        });
        
    // Add shopkeeper to the spawn table with proper probability management
    addMonsterType({
        "Shopkeeper", 8, 1, 0, 0.0f,  // Weight 8, level 1+, no level scaling
        [](Vector2D pos) {
            // Use ShopkeeperFactory's probability system for actual spawning
            if (ShopkeeperFactory::should_spawn_shopkeeper(game.level_manager.get_dungeon_level()))
            {
                auto shopkeeper = ShopkeeperFactory::create_shopkeeper(pos, game.level_manager.get_dungeon_level());
                game.creatures.push_back(std::move(shopkeeper));
                game.log("Shopkeeper spawned via MonsterFactory with proper probability at level " + std::to_string(game.level_manager.get_dungeon_level()));
            }
            else
            {
                // If shopkeeper spawn probability fails, spawn alternative monster
                game.creatures.push_back(std::make_unique<Goblin>(pos));
                game.log("Shopkeeper spawn failed, spawned Goblin instead");
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

void MonsterFactory::spawn_random_monster(Vector2D position, int dungeonLevel)
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
        game.log("No valid monsters for this dungeon level!");
        return;
    }

    // Roll random number and select monster
    int roll = game.d.roll(1, totalWeight);
    int runningTotal = 0;

    for (size_t i = 0; i < monsterTypes.size(); i++) {
        runningTotal += weights[i];
        if (roll <= runningTotal) {
            // Create the monster
            monsterTypes[i].createFunc(position);

            // Check if special AI needed for ranged attackers
            Creature* monster = game.get_actor(position);
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