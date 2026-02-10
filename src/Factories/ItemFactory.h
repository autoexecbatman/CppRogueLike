#pragma once

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

struct Vector2D;
struct GameContext;
struct EnhancedItemSpawnRule;

// A struct to represent an item type with its spawn probability
struct ItemType
{
	std::string name;       // to store in category
    int baseWeight;         // Base weight/probability
    int levelMinimum;       // Minimum dungeon level for this item
    int levelMaximum;       // Maximum dungeon level for this item (0 = no maximum)
    float levelScaling;     // How much to scale weight by level (can be negative)
    std::string category;   // Item category for filtering

    // Factory function to create the item
    std::function<void(Vector2D, GameContext&)> createFunc;
};

struct ItemPercentage
{
	std::string name;
	std::string category;
	float percentage;
};

class ItemFactory
{
public:
    ItemFactory();

    void add_item_type(const ItemType& itemType);
    void load_from_registry();
    void load_enhanced_rules(std::span<const EnhancedItemSpawnRule> rules);
    void generate_treasure(Vector2D position, GameContext& ctx, int dungeonLevel, int quality);
    std::vector<ItemPercentage> get_current_distribution(int dungeonLevel);
    void spawn_item_of_category(Vector2D position, GameContext& ctx, int dungeonLevel, const std::string& category);
    void spawn_random_item(Vector2D position, GameContext& ctx, int dungeonLevel);
    void spawn_all_enhanced_items_debug(Vector2D position, GameContext& ctx);
private:
    std::vector<ItemType> itemTypes;

    // Map of item categories to filter items by type
    std::unordered_map<std::string, std::vector<size_t>> itemCategories;

    int calculate_weight(const ItemType& item, int dungeonLevel) const;
};