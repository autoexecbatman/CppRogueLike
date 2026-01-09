#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>
#include "Factories/MonsterFactory.cpp" // Include the target file specifically

// Mock classes and functions to simulate game environment
class Game {
public:
    class LevelManager {
    public:
        int get_dungeon_level() const { return 1; } // Mock level for testing
    };

    class RandomDice {
    public:
        int roll(int min, int max) { return min + rand() % (max - min + 1); }
    };

    LevelManager level_manager;
    RandomDice d;
    std::vector<std::unique_ptr<Creature>> creatures;

    Creature* get_actor(Vector2D pos) { return nullptr; } // Mock function to simulate getting an actor at a position
};

class Vector2D {};

class Creature {};

namespace InventoryOperations {
    void add_item(Inventory& inventory, ItemPtr item) {}
}

// Mock MonsterType and other necessary structures
struct MonsterType {
    std::string name;
    int baseWeight;
    int levelMinimum;
    int levelMaximum;
    float levelScaling;
    std::function<void(Vector2D)> createFunc;
};

MonsterFactory::MonsterFactory() {
    // Initialize with all monster types (mock implementation)
}

void MonsterFactory::addMonsterType(const MonsterType& monsterType) {
    monsterTypes.push_back(monsterType);
}

int MonsterFactory::calculate_weight(const MonsterType& monster, int dungeonLevel) const {
    // Mock implementation for testing
    return 1; // All monsters have equal weight for simplicity in this mock
}

void MonsterFactory::spawn_random_monster(Vector2D position, int dungeonLevel) {
    // Mock implementation for testing
}

std::vector<std::pair<std::string, float>> MonsterFactory::getCurrentDistribution(int dungeonLevel) {
    // Mock implementation for testing
    return {};
}

// Test functions
void test_MonsterFactoryInitialization() {
    Game game;
    MonsterFactory factory;

    assert(factory.monsterTypes.size() == 10); // Ensure all monsters are initialized

    std::cout << "Test MonsterFactory Initialization Passed!" << std::endl;
}

void test_CalculateWeight() {
    Game game;
    MonsterFactory factory;

    MonsterType mimic = {"Mimic", 6, 2, 0, 0.5f, [](Vector2D pos) {}};
    assert(factory.calculate_weight(mimic, 1) == 7); // Check weight calculation for level 1

    std::cout << "Test Calculate Weight Passed!" << std::endl;
}

void test_SpawnRandomMonster() {
    Game game;
    MonsterFactory factory;
    Vector2D pos(0, 0);

    factory.spawn_random_monster(pos, 1); // Mock call to spawn a monster

    assert(!game.creatures.empty()); // Ensure a creature is spawned

    std::cout << "Test Spawn Random Monster Passed!" << std::endl;
}

void test_GetCurrentDistribution() {
    Game game;
    MonsterFactory factory;

    auto distribution = factory.getCurrentDistribution(1);

    assert(!distribution.empty()); // Ensure there is a distribution

    std::cout << "Test Get Current Distribution Passed!" << std::endl;
}

int main() {
    test_MonsterFactoryInitialization();
    test_CalculateWeight();
    test_SpawnRandomMonster();
    test_GetCurrentDistribution();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}