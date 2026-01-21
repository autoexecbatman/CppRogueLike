#pragma once

#include <memory>
#include <vector>

class Item;
class Container;

// Utility functions for item type operations
namespace ItemTypeUtils
{
    // Find an item in inventory by unique ID
    Item* find_item_by_id(std::vector<std::unique_ptr<Item>>& inventory, int uniqueId);
    const Item* find_item_by_id(const std::vector<std::unique_ptr<Item>>& inventory, int uniqueId);
    
    // Extract an item from container by unique ID (removes from container, returns ownership)
    std::unique_ptr<Item> extract_item_by_id(Container& container, int uniqueId);
}
