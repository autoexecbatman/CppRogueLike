#pragma once

#include "Vector2D.h"
#include "Actor/Actor.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

// A struct to represent an item type with its spawn probability
struct ItemType {
    std::string name;
    int baseWeight;         // Base weight/probability
    int levelMinimum;       // Minimum dungeon level for this item
    int levelMaximum;       // Maximum dungeon level for this item (0 = no maximum)
    float levelScaling;     // How much to scale weight by level (can be negative)
    int valueMin;           // Minimum gold value
    int valueMax;           // Maximum gold value

    // Factory function to create the item
    std::function<void(Vector2D)> createFunc;
};

class ItemFactory {
public:
    ItemFactory();
    ~ItemFactory() = default;

    // Spawn a random item at the given position based on dungeon level
    void spawnRandomItem(Vector2D position, int dungeonLevel);

    // Spawn a specific item category (weapon, potion, scroll, etc.)
    void spawnItemOfCategory(Vector2D position, int dungeonLevel, const std::string& category);

    // Get the probability distribution for the current dungeon level
    std::vector<std::pair<std::string, float>> getCurrentDistribution(int dungeonLevel);

    void generateTreasure(Vector2D position, int dungeonLevel, int quality);

    // Add an item type to the factory
    void addItemType(const ItemType& itemType);

private:
    std::vector<ItemType> itemTypes;

    // Map of item categories to filter items by type
    std::unordered_map<std::string, std::vector<size_t>> itemCategories;

    // Calculate actual weight of an item based on dungeon level
    int calculateWeight(const ItemType& item, int dungeonLevel) const;
};