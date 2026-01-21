#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../Utils/Vector2D.h"
#include "../Actor/Actor.h"

// Forward declarations
struct GameContext;

// A struct to represent a monster type with its spawn probability
struct MonsterType
{
    std::string name;
    int baseWeight;         // Base weight/probability
    int levelMinimum;       // Minimum dungeon level for this monster
    int levelMaximum;       // Maximum dungeon level for this monster (0 = no maximum)
    float levelScaling;     // How much to scale weight by level (can be negative)

    // Factory function to create the monster
    std::function<void(Vector2D, GameContext&)> createFunc;
};

class MonsterFactory
{
public:
    MonsterFactory();
    ~MonsterFactory() = default;

    // Spawn a random monster at the given position based on dungeon level
    void spawn_random_monster(Vector2D position, int dungeonLevel, GameContext& ctx);

    // Get the probability distribution for the current dungeon level
    std::vector<std::pair<std::string, float>> getCurrentDistribution(int dungeonLevel);

    // Add a monster type to the factory
    void addMonsterType(const MonsterType& monsterType);

private:
    std::vector<MonsterType> monsterTypes;

    // Calculate actual weight of a monster based on dungeon level
    int calculate_weight(const MonsterType& monster, int dungeonLevel) const;
};