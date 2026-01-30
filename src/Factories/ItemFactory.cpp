#include "ItemFactory.h"
#include "../Items/Items.h"
#include "../ActorTypes/Gold.h"
#include "../Items/Food.h"
#include "../Random/RandomDice.h"
#include "../Items/Amulet.h"
#include "../Actor/InventoryOperations.h"
#include "ItemCreator.h"
#include "../Core/GameContext.h"
#include "../Systems/LevelManager.h"
#include "../Systems/MessageSystem.h"
#include "../Map/Map.h"

using namespace InventoryOperations; // For clean function calls

ItemFactory::ItemFactory()
{
    // Initialize with all item types

    // HEALING - CLERIC REPLACEMENT (Solo fighter needs reliable healing)
    add_item_type({
        // TODO: Storing types as string is error prone we should remedy this.
        "Health Potion", 50, 1, 0, 0.2f, "potion",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_health_potion(pos));
        }
        });

    // MAGIC UTILITY - WIZARD REPLACEMENT (Solo fighter needs magic support)
    add_item_type({
        "Scroll of Lightning Bolt", 20, 2, 0, 0.2f, "scroll",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_scroll_lightning(pos));
        }
        });

    add_item_type({
        "Scroll of Fireball", 15, 3, 0, 0.3f, "scroll",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_scroll_fireball(pos));
        }
        });

    add_item_type({
        "Scroll of Confusion", 15, 2, 0, 0.2f, "scroll",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_scroll_confusion(pos));
        }
        });

    // WEAPONS - FIGHTER PROGRESSION (Inferior weapons rare, focus on upgrades)
    add_item_type({
        "Dagger", 3, 1, 3, -0.5f, "weapon",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_enhanced_dagger(pos, ItemCreator::determine_enhancement_level(ctx, ctx.level_manager->get_dungeon_level())));
        }
        });

    add_item_type({
        "Short Sword", 5, 1, 4, -0.4f, "weapon",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_enhanced_short_sword(pos, ItemCreator::determine_enhancement_level(ctx, ctx.level_manager->get_dungeon_level())));
        }
        });

    add_item_type({
        "Long Sword", 6, 1, 0, -0.2f, "weapon",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_enhanced_long_sword(pos, ItemCreator::determine_enhancement_level(ctx, ctx.level_manager->get_dungeon_level())));
        }
        });

    add_item_type({
        "Staff", 8, 2, 0, 0.1f, "weapon",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_enhanced_staff(pos, ItemCreator::determine_enhancement_level(ctx, ctx.level_manager->get_dungeon_level())));
        }
        });

    add_item_type({
        "Longbow", 12, 3, 0, 0.3f, "weapon",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_enhanced_longbow(pos, ItemCreator::determine_enhancement_level(ctx, ctx.level_manager->get_dungeon_level())));
        }
        });

    // Gold
    add_item_type({
        "Gold", 25, 1, 0, 0.1f, "gold",
        [](Vector2D pos, GameContext& ctx) { add_item(*ctx.inventory_data, ItemCreator::create_gold_pile(pos, ctx)); }
        });

    // FOOD - SOLO RESOURCE MANAGEMENT (No party sharing, need reliable food)
    add_item_type({
        "Ration", 25, 1, 0, 0.1f, "food",
        [](Vector2D pos, GameContext& ctx) { add_item(*ctx.inventory_data, ItemCreator::create_ration(pos)); }
        });

    add_item_type({
        "Fruit", 15, 1, 0, 0.0f, "food",
        [](Vector2D pos, GameContext& ctx) { add_item(*ctx.inventory_data, ItemCreator::create_fruit(pos)); }
        });

    add_item_type({
        "Bread", 12, 1, 0, 0.0f, "food",
        [](Vector2D pos, GameContext& ctx) { add_item(*ctx.inventory_data, ItemCreator::create_bread(pos)); }
        });

    add_item_type({
        "Meat", 8, 2, 0, 0.1f, "food",
        [](Vector2D pos, GameContext& ctx) { add_item(*ctx.inventory_data, ItemCreator::create_meat(pos)); }
        });

    // Amulet of Yendor - incredibly rare, only appears on deeper levels
    add_item_type({
        "Amulet of Yendor", 1, 8, 0, 2.0f, "artifact",
        [](Vector2D pos, GameContext& ctx) { add_item(*ctx.inventory_data, ItemCreator::create_amulet_of_yendor(pos)); }
        });

    add_item_type({
        "Leather Armor", 2, 1, 0, -0.4f, "armor",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_leather_armor(pos));
        }
        });

    add_item_type({
        "Chain Mail", 3, 3, 0, -0.3f, "armor",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_chain_mail(pos));
        }
        });

    add_item_type({
        "Plate Mail", 1, 5, 0, -0.5f, "armor",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_plate_mail(pos));
        }
        });

    // AUTHENTIC AD&D 2e MAGICAL ITEMS

    // Magical Helms (very rare, special abilities)
    add_item_type({
        "Helm of Brilliance", 1, 6, 0, 0.15f, "magical_helm",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_helm_of_brilliance(pos));
        }
        });

    // Magical Rings (rare, valuable)
    add_item_type({
        "Ring of Protection +1", 2, 3, 0, 0.3f, "magical_ring",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_ring_of_protection_plus_1(pos));
        }
        });

    add_item_type({
        "Ring of Protection +2", 1, 6, 0, 0.4f, "magical_ring",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_ring_of_protection_plus_2(pos));
        }
        });

    add_item_type({
        "Ring of Free Action", 1, 4, 0, 0.3f, "magical_ring",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_ring_of_free_action(pos));
        }
        });

    add_item_type({
        "Ring of Regeneration", 1, 7, 0, 0.5f, "magical_ring",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_ring_of_regeneration(pos));
        }
        });

    add_item_type({
        "Ring of Invisibility", 1, 6, 0, 0.4f, "magical_ring",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_ring_of_invisibility(pos));
        }
        });

    // Gauntlets (rare, stat bonuses)
    add_item_type({
        "Gauntlets of Ogre Power", 1, 5, 0, 0.4f, "gauntlets",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_gauntlets_of_ogre_power(pos));
        }
        });

    add_item_type({
        "Gauntlets of Dexterity", 1, 4, 0, 0.3f, "gauntlets",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_gauntlets_of_dexterity(pos));
        }
        });

    // Girdles (extremely rare, giant strength)
    add_item_type({
        "Girdle of Hill Giant", 1, 6, 0, 0.5f, "girdle",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_girdle_of_hill_giant_strength(pos));
        }
        });

    add_item_type({
        "Girdle of Frost Giant", 1, 8, 0, 0.6f, "girdle",
        [](Vector2D pos, GameContext& ctx)
        {
            add_item(*ctx.inventory_data, ItemCreator::create_girdle_of_frost_giant_strength(pos));
        }
        });

    // Populate item categories from category field
    for (size_t i = 0; i < itemTypes.size(); i++)
    {
        itemCategories[itemTypes[i].category].push_back(i);
    }
}

// Add an item type to the factory
void ItemFactory::add_item_type(const ItemType& itemType)
{
    itemTypes.push_back(itemType);
}

void ItemFactory::generate_treasure(Vector2D position, GameContext& ctx, int dungeonLevel, int quality)
{
    // quality is 1-3: 1=normal, 2=good, 3=exceptional

    // Number of items to generate
    int itemCount = 0;
    switch (quality)
    {

    case 1:
    {
        itemCount = ctx.dice->roll(1, 2);
        break;
    }

    case 2:
    {
        itemCount = ctx.dice->roll(2, 3);
        break;
    }

    case 3:
    {
        itemCount = ctx.dice->roll(3, 5);
        break;
    }

    default: itemCount = 1;
    }

    // Boost dungeon level for item generation to get better items
    int effectiveLevel = dungeonLevel + (quality - 1) * 2;

    // Always include gold with amount based on quality
    int goldMin = 10 * dungeonLevel * quality;
    int goldMax = 20 * dungeonLevel * quality;
    int goldAmount = ctx.dice->roll(goldMin, goldMax);

    // Create gold pile
    auto goldPile = std::make_unique<Item>(position, ActorData{ '$', "gold pile", YELLOW_BLACK_PAIR });
    goldPile->pickable = std::make_unique<Gold>(goldAmount);
    add_item(*ctx.inventory_data, std::move(goldPile));

    // Generate other random items
    for (int i = 0; i < itemCount; i++)
    {
        // Small offset to avoid items on same tile
        Vector2D itemPos = position;
        itemPos.x += ctx.dice->roll(-1, 1);
        itemPos.y += ctx.dice->roll(-1, 1);

        // Ensure position is valid
        if (!ctx.map->can_walk(itemPos, ctx))
        {
            itemPos = position; // Fallback to original position
        }

        // Determine item type with biased probabilities
        int roll = ctx.dice->d100();

        if (quality == 3 && roll <= 5)
        {
            // 5% chance of very special items in exceptional quality treasure
            if (effectiveLevel >= 8 && ctx.dice->d100() <= 10)
            {
                // 10% chance for Amulet at high enough level
                spawn_item_of_category(itemPos, ctx, effectiveLevel, "artifact");
            }
            else
            {
                // High quality weapons
                spawn_item_of_category(itemPos, ctx, effectiveLevel + 2, "weapon");
            }
        }
        else if (roll <= 25)
        {
            // 25% chance of weapon
            spawn_item_of_category(itemPos, ctx, effectiveLevel, "weapon");
        }
        else if (roll <= 50)
        {
            // 25% chance of scroll
            spawn_item_of_category(itemPos, ctx, effectiveLevel, "scroll");
        }
        else if (roll <= 75)
        {
            // 25% chance of potion
            spawn_item_of_category(itemPos, ctx, effectiveLevel, "potion");
        }
        else
        {
            // 25% chance of food
            spawn_item_of_category(itemPos, ctx, effectiveLevel, "food");
        }
    }

    ctx.message_system->log("Generated treasure of quality " + std::to_string(quality) +
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
void ItemFactory::spawn_item_of_category(Vector2D position, GameContext& ctx, int dungeonLevel, const std::string& category)
{
    // Get indices of items in this category
    auto it = itemCategories.find(category);
    if (it == itemCategories.end() || it->second.empty())
    {
        ctx.message_system->log("No items found in category: " + category);
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
        ctx.message_system->log("No valid items in category " + category + " for this dungeon level!");
        return;
    }

    // Roll random number and select item
    int roll = ctx.dice->roll(1, totalWeight);
    int runningTotal = 0;

    for (size_t i = 0; i < indices.size(); i++)
    {
        runningTotal += weights[i];
        if (roll <= runningTotal)
        {
            // Create the item
            itemTypes[indices[i]].createFunc(position, ctx);
            break;
        }
    }
}

// Spawn a random item at the given position based on dungeon level
void ItemFactory::spawn_random_item(Vector2D position, GameContext& ctx, int dungeonLevel)
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
        ctx.message_system->log("No valid items for this dungeon level!");
        return;
    }

    // Roll random number and select item
    int roll = ctx.dice->roll(1, totalWeight);
    int runningTotal = 0;

    for (size_t i = 0; i < itemTypes.size(); i++)
    {
        runningTotal += weights[i];
        if (roll <= runningTotal)
        {
            // Create the item
            itemTypes[i].createFunc(position, ctx);
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