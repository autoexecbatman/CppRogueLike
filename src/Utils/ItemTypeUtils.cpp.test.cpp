#include "Actor/Container.h"
#include "Actor/Actor.h"
#include <cassert>
#include <iostream>
#include "ItemTypeUtils.h" // Include the header file for ItemTypeUtils
#include "../Actor/Container.h" // Include the Container class definition
#include <memory> // For std::unique_ptr
#include <vector> // For std::vector

// Mocking up some classes and functions that are used in ItemTypeUtils but not defined here
class Item {
public:
    int uniqueId;
    Item(int id) : uniqueId(id) {}
};

namespace ItemTypeUtils {
    Item* find_item_by_id(std::vector<std::unique_ptr<Item>>& inventory, int uniqueId) {
        for (auto& item : inventory) {
            if (item->uniqueId == uniqueId) {
                return item.get();
            }
        }
        return nullptr;
    }
}

int main() {
    // Setup the test environment
    std::vector<std::unique_ptr<Item>> inventory;
    inventory.push_back(std::make_unique<Item>(1));
    inventory.push_back(std::make_unique<Item>(2));
    inventory.push_back(std::make_unique<Item>(3));

    // Test find_item_by_id (non-const version)
    {
        Item* foundItem = ItemTypeUtils::find_item_by_id(inventory, 2);
        assert(foundItem != nullptr);
        assert(foundItem->uniqueId == 2);
        std::cout << "Test find_item_by_id (non-const) passed." << std::endl;
    }

    // Test find_item_by_id (const version)
    {
        const auto& constInventory = inventory;
        const Item* foundItem = ItemTypeUtils::find_item_by_id(constInventory, 2);
        assert(foundItem != nullptr);
        assert(foundItem->uniqueId == 2);
        std::cout << "Test find_item_by_id (const) passed." << std::endl;
    }

    // Test extract_item_by_id
    {
        Container container;
        container.set_inventory(std::move(inventory));
        auto extractedItem = ItemTypeUtils::extract_item_by_id(container, 2);
        assert(extractedItem != nullptr);
        assert(extractedItem->uniqueId == 2);
        std::cout << "Test extract_item_by_id passed." << std::endl;
    }

    // Additional tests can be added here to cover more edge cases or different scenarios.

    return 0;
}