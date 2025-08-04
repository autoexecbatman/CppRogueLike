#pragma once

#include "../Utils/Vector2D.h"
#include "../Actor/Actor.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

// A struct to represent an item type with its spawn probability
struct ItemType
{
	std::string name;       // to store in category
    int baseWeight;         // Base weight/probability
    int levelMinimum;       // Minimum dungeon level for this item
    int levelMaximum;       // Maximum dungeon level for this item (0 = no maximum)
    float levelScaling;     // How much to scale weight by level (can be negative)

    // Factory function to create the item
    std::function<void(Vector2D)> createFunc;
};

struct ItemPercentage
{
	std::string name;       // to store in category
	float percentage;       // Percentage of this item type
};

class ItemFactory
{
public:
    ItemFactory();

    void add_item_type(const ItemType& itemType);
    void generate_treasure(Vector2D position, int dungeonLevel, int quality);
    std::vector<ItemPercentage> get_current_distribution(int dungeonLevel);
    void spawn_item_of_category(Vector2D position, int dungeonLevel, const std::string& category);
    void spawn_random_item(Vector2D position, int dungeonLevel);
private:
    std::vector<ItemType> itemTypes;

    // Map of item categories to filter items by type
    std::unordered_map<std::string, std::vector<size_t>> itemCategories;

    int calculate_weight(const ItemType& item, int dungeonLevel) const;
};