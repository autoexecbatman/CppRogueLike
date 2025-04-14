#include "ItemFactory.h"
#include "Game.h"
#include "Items.h"
#include "ActorTypes/Gold.h"
#include "Food.h"
#include "Random/RandomDice.h"
#include "Amulet.h"

ItemFactory::ItemFactory() {
    // Initialize with all item types

    // Potions
    add_item_type({
        "Health Potion", 30, 1, 0, -0.2f,
        [](Vector2D pos) { game.create_item<HealthPotion>(pos); }
        });

    // Scrolls
    add_item_type({
        "Scroll of Lightning Bolt", 15, 2, 0, 0.1f,
        [](Vector2D pos) { game.create_item<ScrollOfLightningBolt>(pos); }
        });

    add_item_type({
        "Scroll of Fireball", 10, 3, 0, 0.2f,
        [](Vector2D pos) { game.create_item<ScrollOfFireball>(pos); }
        });

    add_item_type({
        "Scroll of Confusion", 10, 2, 0, 0.1f,
        [](Vector2D pos) { game.create_item<ScrollOfConfusion>(pos); }
        });

    // Weapons
    add_item_type({
        "Dagger", 15, 1, 5, -0.3f,
        [](Vector2D pos) {
            auto item = std::make_unique<Item>(pos, ActorData{ '/', "dagger", 1 });
            item->pickable = std::make_unique<Dagger>();
            game.container->add(std::move(item));
        }
        });

    add_item_type({
        "Short Sword", 10, 2, 0, -0.1f,
        [](Vector2D pos) {
            auto item = std::make_unique<Item>(pos, ActorData{ '/', "short sword", 1 });
            item->pickable = std::make_unique<ShortSword>();
            game.container->add(std::move(item));
        }
        });

    add_item_type({
        "Long Sword", 8, 3, 0, 0.2f,
        [](Vector2D pos) {
            auto item = std::make_unique<Item>(pos, ActorData{ '/', "long sword", 1 });
            item->pickable = std::make_unique<LongSword>();
            game.container->add(std::move(item));
        }
        });

    add_item_type({
        "Staff", 7, 3, 0, 0.1f,
        [](Vector2D pos) {
            auto item = std::make_unique<Item>(pos, ActorData{ '/', "staff", 1 });
            item->pickable = std::make_unique<Staff>();
            game.container->add(std::move(item));
        }
        });

    add_item_type({
        "Longbow", 6, 4, 0, 0.2f,
        [](Vector2D pos) {
            auto item = std::make_unique<Item>(pos, ActorData{ ')', "longbow", 1 });
            item->pickable = std::make_unique<Longbow>();
            game.container->add(std::move(item));
        }
        });

    // Gold
    add_item_type({
        "Gold", 25, 1, 0, 0.1f,
        [](Vector2D pos) { game.create_item<GoldPile>(pos); }
        });

    // Food
    add_item_type({
        "Ration", 15, 1, 0, -0.1f,
        [](Vector2D pos) { game.create_item<Ration>(pos); }
        });

    add_item_type({
        "Fruit", 10, 1, 0, -0.2f,
        [](Vector2D pos) { game.create_item<Fruit>(pos); }
        });

    add_item_type({
        "Bread", 8, 1, 0, -0.1f,
        [](Vector2D pos) { game.create_item<Bread>(pos); }
        });

    add_item_type({
        "Meat", 5, 2, 0, 0.0f,
        [](Vector2D pos) { game.create_item<Meat>(pos); }
        });

    // Amulet of Yendor - incredibly rare, only appears on deeper levels
    add_item_type({
        "Amulet of Yendor", 1, 8, 0, 2.0f,
        [](Vector2D pos) { game.create_item<AmuletOfYendor>(pos); }
        });

    add_item_type({
    "Leather Armor", 15, 1, 0, -0.1f,
    [](Vector2D pos) { game.create_item<LeatherArmorItem>(pos); }
        });

    // Populate item categories
    for (size_t i = 0; i < itemTypes.size(); i++) {
        const auto& item = itemTypes[i];
        std::string category;

        if (item.name.find("Potion") != std::string::npos) {
            category = "potion";
        }
        else if (item.name.find("Scroll") != std::string::npos) {
            category = "scroll";
        }
        else if (item.name == "Gold") {
            category = "gold";
        }
        else if (item.name == "Amulet of Yendor") {
            category = "artifact";
        }
        else if (item.name.find("Ration") != std::string::npos ||
            item.name.find("Fruit") != std::string::npos ||
            item.name.find("Bread") != std::string::npos ||
            item.name.find("Meat") != std::string::npos) {
            category = "food";
        }
        else if (item.name.find("Armor") != std::string::npos) {
            category = "armor";
        }
        else {
            category = "weapon";
        }

        itemCategories[category].push_back(i);
    }
}

// Add an item type to the factory
void ItemFactory::add_item_type(const ItemType& itemType)
{
    itemTypes.push_back(itemType);
}

void ItemFactory::generate_treasure(Vector2D position, int dungeonLevel, int quality)
{
    // quality is 1-3: 1=normal, 2=good, 3=exceptional

    // Number of items to generate
    int itemCount = 0;
    switch (quality)
    {

    case 1:
    {
        itemCount = game.d.roll(1, 2);
        break;
    }

    case 2:
    {
        itemCount = game.d.roll(2, 3);
        break;
    }

    case 3:
    {
        itemCount = game.d.roll(3, 5);
        break;
    }

    default: itemCount = 1;
    }

    // Boost dungeon level for item generation to get better items
    int effectiveLevel = dungeonLevel + (quality - 1) * 2;

    // Always include gold with amount based on quality
    int goldMin = 10 * dungeonLevel * quality;
    int goldMax = 20 * dungeonLevel * quality;
    int goldAmount = game.d.roll(goldMin, goldMax);

    // Create gold pile
    auto goldPile = std::make_unique<Item>(position, ActorData{ '$', "gold pile", GOLD_PAIR });
    goldPile->pickable = std::make_unique<Gold>(goldAmount);
    game.container->add(std::move(goldPile));

    // Generate other random items
    for (int i = 0; i < itemCount; i++)
    {
        // Small offset to avoid items on same tile
        Vector2D itemPos = position;
        itemPos.x += game.d.roll(-1, 1);
        itemPos.y += game.d.roll(-1, 1);

        // Ensure position is valid
        if (!game.map->can_walk(itemPos))
        {
            itemPos = position; // Fallback to original position
        }

        // Determine item type with biased probabilities
        int roll = game.d.d100();

        if (quality == 3 && roll <= 5)
        {
            // 5% chance of very special items in exceptional quality treasure
            if (effectiveLevel >= 8 && game.d.d100() <= 10)
            {
                // 10% chance for Amulet at high enough level
                spawn_item_of_category(itemPos, effectiveLevel, "artifact");
            }
            else
            {
                // High quality weapons
                spawn_item_of_category(itemPos, effectiveLevel + 2, "weapon");
            }
        }
        else if (roll <= 25)
        {
            // 25% chance of weapon
            spawn_item_of_category(itemPos, effectiveLevel, "weapon");
        }
        else if (roll <= 50)
        {
            // 25% chance of scroll
            spawn_item_of_category(itemPos, effectiveLevel, "scroll");
        }
        else if (roll <= 75)
        {
            // 25% chance of potion
            spawn_item_of_category(itemPos, effectiveLevel, "potion");
        }
        else
        {
            // 25% chance of food
            spawn_item_of_category(itemPos, effectiveLevel, "food");
        }
    }

    game.log("Generated treasure of quality " + std::to_string(quality) +
        " with " + std::to_string(itemCount + 1) + " items including gold");
}

// Get the probability distribution for the current dungeon level
std::vector<ItemPercentage> ItemFactory::get_current_distribution(int dungeonLevel)
{
    std::vector<ItemPercentage> distribution;

    // Calculate total weights for current dungeon level
    int totalWeight = 0;
    std::vector<int> weights;

    for (const auto& item : itemTypes)
    {
        int weight = calculate_weight(item, dungeonLevel);
        weights.push_back(weight);
        totalWeight += weight;
    }

    // Calculate percentage for each item
    for (size_t i = 0; i < itemTypes.size(); i++)
    {
        if (weights[i] > 0)
        {
            float percentage = static_cast<float>(weights[i]) / totalWeight * 100.0f;
            distribution.push_back(ItemPercentage{ itemTypes[i].name, percentage });
        }
    }

    return distribution;
}

// Spawn a specific item category (weapon, potion, scroll, etc.)
void ItemFactory::spawn_item_of_category(Vector2D position, int dungeonLevel, const std::string& category)
{
    // Get indices of items in this category
    auto it = itemCategories.find(category);
    if (it == itemCategories.end() || it->second.empty())
    {
        game.log("No items found in category: " + category);
        return;
    }

    const auto& indices = it->second;

    // Calculate total weights for current dungeon level in this category
    int totalWeight = 0;
    std::vector<int> weights;

    for (size_t idx : indices)
    {
        int weight = calculate_weight(itemTypes[idx], dungeonLevel);
        weights.push_back(weight);
        totalWeight += weight;
    }

    // If no valid items for this level in this category, do nothing
    if (totalWeight <= 0)
    {
        game.log("No valid items in category " + category + " for this dungeon level!");
        return;
    }

    // Roll random number and select item
    int roll = game.d.roll(1, totalWeight);
    int runningTotal = 0;

    for (size_t i = 0; i < indices.size(); i++)
    {
        runningTotal += weights[i];
        if (roll <= runningTotal)
        {
            // Create the item
            itemTypes[indices[i]].createFunc(position);
            break;
        }
    }
}

// Spawn a random item at the given position based on dungeon level
void ItemFactory::spawn_random_item(Vector2D position, int dungeonLevel)
{
    // Calculate total weights for current dungeon level
    int totalWeight = 0;
    std::vector<int> weights;

    for (const auto& item : itemTypes)
    {
        int weight = calculate_weight(item, dungeonLevel);
        weights.push_back(weight);
        totalWeight += weight;
    }

    // If no valid items for this level, do nothing
    if (totalWeight <= 0)
    {
        game.log("No valid items for this dungeon level!");
        return;
    }

    // Roll random number and select item
    int roll = game.d.roll(1, totalWeight);
    int runningTotal = 0;

    for (size_t i = 0; i < itemTypes.size(); i++)
    {
        runningTotal += weights[i];
        if (roll <= runningTotal)
        {
            // Create the item
            itemTypes[i].createFunc(position);
            break;
        }
    }
}

// Calculate actual weight of an item based on dungeon level
int ItemFactory::calculate_weight(const ItemType& item, int dungeonLevel) const
{
    // Check level requirements
    if (dungeonLevel < item.levelMinimum ||
        (item.levelMaximum > 0 && dungeonLevel > item.levelMaximum))
    {
        return 0;
    }

    // Calculate scaled weight based on dungeon level
    float levelFactor = 1.0f + (item.levelScaling * (dungeonLevel - 1));
    int weight = static_cast<int>(item.baseWeight * levelFactor);

    // Ensure weight is at least 1 if item is available at this level
    return std::max(1, weight);
}