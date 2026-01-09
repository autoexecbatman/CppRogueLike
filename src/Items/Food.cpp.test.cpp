#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>

// Include the target file specifically
#include "E:/repo/C++RogueLike/src/Items/Food.cpp"
#include "E:/repo/C++RogueLike/src/Game.h"
#include "E:/repo/C++RogueLike/src/Colors/Colors.h"

// Mock classes to simulate dependencies
class Game {
public:
    void append_message_part(std::pair<int, int> colorPair, const std::string& message) {
        messages.push_back({colorPair, message});
    }

    void finalize_message() {
        for (const auto& msg : messages) {
            std::cout << "\033[" << msg.first.first << ";" << msg.first.second << "m" << msg.second;
        }
        messages.clear();
    }

    struct HungerSystem {
        void decrease_hunger(int amount) {}
    };

    HungerSystem hunger_system;
private:
    std::vector<std::pair<std::pair<int, int>, std::string>> messages;
};

class Item {
public:
    struct ActorData {
        char glyph;
        std::string name;
        std::pair<int, int> colorPair;
    };

    ActorData actorData;
    Game& game; // Assuming this is accessible for testing purposes

    Item(Vector2D position, ActorData actorData) : actorData(actorData), game(*new Game()) {}
};

class Creature {
public:
    void take_damage(int amount) {}
};

void test_FoodConstruction() {
    Food food1(300);
    assert(food1.nutrition_value == 300 && "Food construction failed");

    Food food2(100);
    assert(food2.nutrition_value == 100 && "Food construction failed");
}

void test_FoodUse() {
    Game game;
    Item::ActorData actorData = {'%', "test_food", std::make_pair(7, 0)};
    Food food(200);
    Item item(Vector2D(), actorData);
    Creature creature;

    bool result = food.use(item, creature);
    assert(result && "Food use failed");
}

void test_FoodSerialization() {
    json j;
    Food food(400);
    food.save(j);
    assert(j["nutrition_value"] == 400 && "Nutrition value serialization failed");
    assert(j["type"] == static_cast<int>(PickableType::FOOD) && "Type serialization failed");
}

void test_FoodDeserialization() {
    json j = R"(
        {"nutrition_value": 500, "type": 1}
    )"_json;
    Food food(0);
    food.load(j);
    assert(food.nutrition_value == 500 && "Nutrition value deserialization failed");
}

int main() {
    test_FoodConstruction();
    test_FoodUse();
    test_FoodSerialization();
    test_FoodDeserialization();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}