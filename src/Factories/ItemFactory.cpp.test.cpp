#include "Factories/ItemFactory.cpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <stdexcept>

// Assuming the necessary includes for ItemFactory, Vector2D, etc., are present here
// #include "ItemFactory.h"
// #include "../Game.h"
// #include "../Items/Items.h"
// #include "../ActorTypes/Gold.h"
// #include "../Items/Food.h"
// #include "../Random/RandomDice.h"
// #include "../Items/Amulet.h"
// #include "../Actor/InventoryOperations.h"
// #include "ItemCreator.h"

using namespace InventoryOperations; // For clean function calls

void test_generate_treasure() {
    // Setup a basic game environment for testing
    Game game; // Assuming Game class is defined somewhere with necessary methods
    ItemFactory itemFactory;
    Vector2D position(0, 0);
    int dungeonLevel = 1;
    int quality = 1;

    // Test generate_treasure method
    itemFactory.generate_treasure(position, dungeonLevel, quality);

    // Add assertions to check if items are generated correctly
    assert(!game.inventory_data.empty()); // Ensure inventory is not empty after generation
}

void test_get_current_distribution() {
    ItemFactory itemFactory;
    int dungeonLevel = 1;

    auto distribution = itemFactory.get_current_distribution(dungeonLevel);

    // Add assertions to check the structure and content of the returned distribution
    assert(!distribution.empty()); // Ensure there are items in the distribution
}

void test_spawn_item_of_category() {
    ItemFactory itemFactory;
    Vector2D position(0, 0);
    int dungeonLevel = 1;
    std::string category = "weapon";

    itemFactory.spawn_item_of_category(position, dungeonLevel, category);

    // Add assertions to check if items are spawned correctly in the specified category
    assert(!game.inventory_data.empty()); // Ensure inventory is not empty after spawning
}

void test_spawn_random_item() {
    ItemFactory itemFactory;
    Vector2D position(0, 0);
    int dungeonLevel = 1;

    itemFactory.spawn_random_item(position, dungeonLevel);

    // Add assertions to check if a random item is spawned correctly
    assert(!game.inventory_data.empty()); // Ensure inventory is not empty after spawning
}

int main() {
    try {
        test_generate_treasure();
        std::cout << "test_generate_treasure passed!" << std::endl;
        test_get_current_distribution();
        std::cout << "test_get_current_distribution passed!" << std::endl;
        test_spawn_item_of_category();
        std::cout << "test_spawn_item_of_category passed!" << std::endl;
        test_spawn_random_item();
        std::cout << "test_spawn_random_item passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}