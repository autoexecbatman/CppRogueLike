#include "Items/CorpseFood.cpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>
#include "CorpseFood.h"
#include "../Game.h"
#include "../Colors/Colors.h"
#include <unordered_map>

// Mock classes to simulate dependencies and fulfill the code's requirements
class Item {
public:
    ActorData actorData;
};

class Creature {};

class Game {
public:
    HungerSystem hunger_system;
    void append_message_part(int colorPair, const std::string& text) {
        // Mock implementation for testing purposes
    }
    Dice d;
    void finalize_message() {}
};

class ActorData {
public:
    std::string name = "default";
};

class HungerSystem {
public:
    void decrease_hunger(int amount) {}
};

class Dice {
public:
    int roll(int min, int max) { return 0; } // Mock dice roll function
};

// Define nutrition values for different monster types
const std::unordered_map<std::string, int> CORPSE_NUTRITION_VALUES = {
    {"dead goblin", 40},
    {"dead orc", 80},
    {"dead troll", 120},
    {"dead dragon", 200},
    {"dead archer", 70},
    {"dead mage", 60},
    {"dead shopkeeper", 100},
};

void test_CorpseFoodConstructor() {
    CorpseFood cf(100);
    assert(cf.nutrition_value == 100 && "Failed to initialize with given nutrition value");
}

void test_CorpseFoodNutritionCalculation() {
    Game game;
    CorpseFood cf(0); // Nutrition value set to 0, should calculate from CORPSE_NUTRITION_VALUES
    Item item;
    Creature creature;

    for (const auto& pair : CORPSE_NUTRITION_VALUES) {
        item.actorData.name = pair.first;
        cf.game = &game; // Assign the mock game object
        cf.use(item, creature);
        assert(cf.nutrition_value == pair.second && "Incorrect nutrition value calculated");
    }
}

void test_CorpseFoodUse() {
    Game game;
    CorpseFood cf(100); // Nutrition value set to 100, no need for calculation
    Item item;
    Creature creature;
    item.actorData.name = "dead goblin";
    cf.game = &game; // Assign the mock game object

    bool result = cf.use(item, creature);
    assert(result && "Failed to use CorpseFood");
}

void test_CorpseFoodLoadAndSave() {
    json j;
    CorpseFood cf(0);
    cf.load(j);
    assert(cf.nutrition_value == 50 && "Default nutrition value not loaded correctly");

    j["nutrition_value"] = 200;
    cf.load(j);
    assert(cf.nutrition_value == 200 && "Nutrition value not loaded correctly from JSON");

    json savedJson;
    cf.save(savedJson);
    assert(savedJson["nutrition_value"] == 200 && "Nutrition value not saved correctly to JSON");
}

int main() {
    test_CorpseFoodConstructor();
    test_CorpseFoodNutritionCalculation();
    test_CorpseFoodUse();
    test_CorpseFoodLoadAndSave();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}