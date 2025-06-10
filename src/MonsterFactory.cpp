// file: MonsterFactory.cpp
#include "MonsterFactory.h"
#include "Game.h"
#include "ActorTypes/Monsters.h"
#include "Spider.h"
#include "Random/RandomDice.h"
#include "AiMonsterRanged.h"
#include "ActorTypes/Healer.h"
#include "ActorTypes/LightningBolt.h"
#include "ActorTypes/Fireball.h"
#include "Food.h"

MonsterFactory::MonsterFactory() {
    // Initialize with all monster types
    addMonsterType({
        "Mimic", 6, 2, 0, 0.5f,
        [](Vector2D pos) { game.create_creature<Mimic>(pos); }
        });

    addMonsterType({
        "Small Spider", 10, 1, 0, -0.3f,
        [](Vector2D pos) { game.create_creature<SmallSpider>(pos); }
        });

    addMonsterType({
        "Giant Spider", 10, 2, 0, 0.0f,
        [](Vector2D pos) { game.create_creature<GiantSpider>(pos); }
        });

    addMonsterType({
        "Web Spinner", 5, 3, 0, 0.2f,
        [](Vector2D pos) { game.create_creature<WebSpinner>(pos); }
        });

    addMonsterType({
        "Goblin", 30, 1, 5, -0.2f,
        [](Vector2D pos) { game.create_creature<Goblin>(pos); }
        });

    addMonsterType({
        "Orc", 15, 2, 0, 0.0f,
        [](Vector2D pos) { game.create_creature<Orc>(pos); }
        });

    addMonsterType({
        "Archer", 10, 2, 0, 0.2f,
        [](Vector2D pos) { game.create_creature<Archer>(pos); }
        });

    addMonsterType({
        "Mage", 10, 3, 0, 0.3f,
        [](Vector2D pos) { game.create_creature<Mage>(pos); }
        });

    addMonsterType({
        "Troll", 7, 4, 0, 0.5f,
        [](Vector2D pos) { game.create_creature<Troll>(pos); }
        });

    addMonsterType({
        "Shopkeeper", 8, 1, 0, 0.0f,  // INCREASED from 5 to 8 - more shopkeepers
        [](Vector2D pos) { 
            game.create_creature<Shopkeeper>(pos);
            // Get the shopkeeper that was just created (last in creatures vector)
            auto& shopkeeper = *game.creatures.back();
            
            // Give shopkeeper starting inventory with meaningful items
            auto healthPotion1 = std::make_unique<Item>(pos, ActorData{'!', "health potion", HPBARMISSING_PAIR});
            healthPotion1->pickable = std::make_unique<Healer>(10);
            healthPotion1->value = 50;
            shopkeeper.container->add(std::move(healthPotion1));
            
            auto healthPotion2 = std::make_unique<Item>(pos, ActorData{'!', "health potion", HPBARMISSING_PAIR});
            healthPotion2->pickable = std::make_unique<Healer>(10);
            healthPotion2->value = 50;
            shopkeeper.container->add(std::move(healthPotion2));
            
            auto scrollLightning = std::make_unique<Item>(pos, ActorData{'#', "scroll of lightning", LIGHTNING_PAIR});
            scrollLightning->pickable = std::make_unique<LightningBolt>(5, 20);
            scrollLightning->value = 150;
            shopkeeper.container->add(std::move(scrollLightning));
            
            auto scrollFireball = std::make_unique<Item>(pos, ActorData{'#', "scroll of fireball", FIREBALL_PAIR});
            scrollFireball->pickable = std::make_unique<Fireball>(3, 12);
            scrollFireball->value = 100;
            shopkeeper.container->add(std::move(scrollFireball));
            
            auto ration = std::make_unique<Item>(pos, ActorData{'%', "ration", DOOR_PAIR});
            ration->pickable = std::make_unique<Food>(50);  // 50 nutrition value
            ration->value = 30;
            shopkeeper.container->add(std::move(ration));
            
            shopkeeper.gold = 500;  // Give shopkeeper gold to buy player items
        }
        });
}

void MonsterFactory::addMonsterType(const MonsterType& monsterType) {
    monsterTypes.push_back(monsterType);
}

int MonsterFactory::calculateWeight(const MonsterType& monster, int dungeonLevel) const {
    // Check level requirements
    if (dungeonLevel < monster.levelMinimum ||
        (monster.levelMaximum > 0 && dungeonLevel > monster.levelMaximum)) {
        return 0;
    }

    // Calculate scaled weight based on dungeon level
    float levelFactor = 1.0f + (monster.levelScaling * (dungeonLevel - 1));
    int weight = static_cast<int>(monster.baseWeight * levelFactor);

    // Ensure weight is at least 1 if monster is available at this level
    return std::max(1, weight);
}

void MonsterFactory::spawnRandomMonster(Vector2D position, int dungeonLevel) {
    // Calculate total weights for current dungeon level
    int totalWeight = 0;
    std::vector<int> weights;

    for (const auto& monster : monsterTypes) {
        int weight = calculateWeight(monster, dungeonLevel);
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
        int weight = calculateWeight(monster, dungeonLevel);
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