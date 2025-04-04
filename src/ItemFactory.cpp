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
    addItemType({
        "Health Potion", 30, 1, 0, -0.2f, 10, 20,
        [](Vector2D pos) { game.create_item<HealthPotion>(pos); }
        });

    // Scrolls
    addItemType({
        "Scroll of Lightning Bolt", 15, 2, 0, 0.1f, 20, 30,
        [](Vector2D pos) { game.create_item<ScrollOfLightningBolt>(pos); }
        });

    addItemType({
        "Scroll of Fireball", 10, 3, 0, 0.2f, 30, 50,
        [](Vector2D pos) { game.create_item<ScrollOfFireball>(pos); }
        });

    addItemType({
        "Scroll of Confusion", 10, 2, 0, 0.1f, 20, 35,
        [](Vector2D pos) { game.create_item<ScrollOfConfusion>(pos); }
        });

    // Weapons
    addItemType({
        "Dagger", 15, 1, 5, -0.3f, 10, 20,
        [](Vector2D pos) {
            auto item = std::make_unique<Item>(pos, ActorData{ '/', "dagger", 1 });
            item->pickable = std::make_unique<Dagger>();
            game.container->add(std::move(item));
        }
        });

    addItemType({
        "Short Sword", 10, 2, 0, -0.1f, 20, 40,
        [](Vector2D pos) {
            auto item = std::make_unique<Item>(pos, ActorData{ '/', "short sword", 1 });
            item->pickable = std::make_unique<ShortSword>();
            game.container->add(std::move(item));
        }
        });

    addItemType({
        "Long Sword", 8, 3, 0, 0.2f, 40, 70,
        [](Vector2D pos) {
            auto item = std::make_unique<Item>(pos, ActorData{ '/', "long sword", 1 });
            item->pickable = std::make_unique<LongSword>();
            game.container->add(std::move(item));
        }
        });

    addItemType({
        "Staff", 7, 3, 0, 0.1f, 25, 50,
        [](Vector2D pos) {
            auto item = std::make_unique<Item>(pos, ActorData{ '/', "staff", 1 });
            item->pickable = std::make_unique<Staff>();
            game.container->add(std::move(item));
        }
        });

    addItemType({
        "Longbow", 6, 4, 0, 0.2f, 50, 80,
        [](Vector2D pos) {
            auto item = std::make_unique<Item>(pos, ActorData{ ')', "longbow", 1 });
            item->pickable = std::make_unique<Longbow>();
            game.container->add(std::move(item));
        }
        });

    // Gold
    addItemType({
        "Gold", 25, 1, 0, 0.1f, 5, 20,
        [](Vector2D pos) { game.create_item<GoldPile>(pos); }
        });

    // Food
    addItemType({
        "Ration", 15, 1, 0, -0.1f, 10, 15,
        [](Vector2D pos) { game.create_item<Ration>(pos); }
        });

    addItemType({
        "Fruit", 10, 1, 0, -0.2f, 3, 7,
        [](Vector2D pos) { game.create_item<Fruit>(pos); }
        });

    addItemType({
        "Bread", 8, 1, 0, -0.1f, 5, 10,
        [](Vector2D pos) { game.create_item<Bread>(pos); }
        });

    addItemType({
        "Meat", 5, 2, 0, 0.0f, 8, 15,
        [](Vector2D pos) { game.create_item<Meat>(pos); }
        });

    // Amulet of Yendor - incredibly rare, only appears on deeper levels
    addItemType({
        "Amulet of Yendor", 1, 8, 0, 2.0f, 1000, 1000,
        [](Vector2D pos) { game.create_item<AmuletOfYendor>(pos); }
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
        else {
            category = "weapon";
        }

        itemCategories[category].push_back(i);
    }
}

void ItemFactory::addItemType(const ItemType& itemType) {
    itemTypes.push_back(itemType);
}

int ItemFactory::calculateWeight(const ItemType& item, int dungeonLevel) const {
    // Check level requirements
    if (dungeonLevel < item.levelMinimum ||
        (item.levelMaximum > 0 && dungeonLevel > item.levelMaximum)) {
        return 0;
    }

    // Calculate scaled weight based on dungeon level
    float levelFactor = 1.0f + (item.levelScaling * (dungeonLevel - 1));
    int weight = static_cast<int>(item.baseWeight * levelFactor);

    // Ensure weight is at least 1 if item is available at this level
    return std::max(1, weight);
}

void ItemFactory::spawnRandomItem(Vector2D position, int dungeonLevel) {
    // Calculate total weights for current dungeon level
    int totalWeight = 0;
    std::vector<int> weights;

    for (const auto& item : itemTypes) {
        int weight = calculateWeight(item, dungeonLevel);
        weights.push_back(weight);
        totalWeight += weight;
    }

    // If no valid items for this level, do nothing
    if (totalWeight <= 0) {
        game.log("No valid items for this dungeon level!");
        return;
    }

    // Roll random number and select item
    int roll = game.d.roll(1, totalWeight);
    int runningTotal = 0;

    for (size_t i = 0; i < itemTypes.size(); i++) {
        runningTotal += weights[i];
        if (roll <= runningTotal) {
            // Create the item
            itemTypes[i].createFunc(position);
            break;
        }
    }
}

void ItemFactory::spawnItemOfCategory(Vector2D position, int dungeonLevel, const std::string& category) {
    // Get indices of items in this category
    auto it = itemCategories.find(category);
    if (it == itemCategories.end() || it->second.empty()) {
        game.log("No items found in category: " + category);
        return;
    }

    const auto& indices = it->second;

    // Calculate total weights for current dungeon level in this category
    int totalWeight = 0;
    std::vector<int> weights;

    for (size_t idx : indices) {
        int weight = calculateWeight(itemTypes[idx], dungeonLevel);
        weights.push_back(weight);
        totalWeight += weight;
    }

    // If no valid items for this level in this category, do nothing
    if (totalWeight <= 0) {
        game.log("No valid items in category " + category + " for this dungeon level!");
        return;
    }

    // Roll random number and select item
    int roll = game.d.roll(1, totalWeight);
    int runningTotal = 0;

    for (size_t i = 0; i < indices.size(); i++) {
        runningTotal += weights[i];
        if (roll <= runningTotal) {
            // Create the item
            itemTypes[indices[i]].createFunc(position);
            break;
        }
    }
}

std::vector<std::pair<std::string, float>> ItemFactory::getCurrentDistribution(int dungeonLevel) {
    std::vector<std::pair<std::string, float>> distribution;

    // Calculate total weights for current dungeon level
    int totalWeight = 0;
    std::vector<int> weights;

    for (const auto& item : itemTypes) {
        int weight = calculateWeight(item, dungeonLevel);
        weights.push_back(weight);
        totalWeight += weight;
    }

    // Calculate percentage for each item
    for (size_t i = 0; i < itemTypes.size(); i++) {
        if (weights[i] > 0) {
            float percentage = static_cast<float>(weights[i]) / totalWeight * 100.0f;
            distribution.push_back({ itemTypes[i].name, percentage });
        }
    }

    return distribution;
}

void ItemFactory::generateTreasure(Vector2D position, int dungeonLevel, int quality) {
    // quality is 1-3: 1=normal, 2=good, 3=exceptional

    // Number of items to generate
    int itemCount = 0;
    switch (quality) {
    case 1: itemCount = game.d.roll(1, 2); break;
    case 2: itemCount = game.d.roll(2, 3); break;
    case 3: itemCount = game.d.roll(3, 5); break;
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
    for (int i = 0; i < itemCount; i++) {
        // Small offset to avoid items on same tile
        Vector2D itemPos = position;
        itemPos.x += game.d.roll(-1, 1);
        itemPos.y += game.d.roll(-1, 1);

        // Ensure position is valid
        if (!game.map->can_walk(itemPos)) {
            itemPos = position; // Fallback to original position
        }

        // Determine item type with biased probabilities
        int roll = game.d.d100();

        if (quality == 3 && roll <= 5) {
            // 5% chance of very special items in exceptional quality treasure
            if (effectiveLevel >= 8 && game.d.d100() <= 10) {
                // 10% chance for Amulet at high enough level
                spawnItemOfCategory(itemPos, effectiveLevel, "artifact");
            }
            else {
                // High quality weapons
                spawnItemOfCategory(itemPos, effectiveLevel + 2, "weapon");
            }
        }
        else if (roll <= 25) {
            // 25% chance of weapon
            spawnItemOfCategory(itemPos, effectiveLevel, "weapon");
        }
        else if (roll <= 50) {
            // 25% chance of scroll
            spawnItemOfCategory(itemPos, effectiveLevel, "scroll");
        }
        else if (roll <= 75) {
            // 25% chance of potion
            spawnItemOfCategory(itemPos, effectiveLevel, "potion");
        }
        else {
            // 25% chance of food
            spawnItemOfCategory(itemPos, effectiveLevel, "food");
        }
    }

    game.log("Generated treasure of quality " + std::to_string(quality) +
        " with " + std::to_string(itemCount + 1) + " items including gold");
}