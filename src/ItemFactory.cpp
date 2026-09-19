#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "Colors.h"
#include "GameContext.h"
#include "InventoryOperations.h"
#include "Item.h"
#include "ItemCreator.h"
#include "ItemEnhancements.h"
#include "ItemFactory.h"
#include "ItemRegistry.h"
#include "LevelManager.h"
#include "Map.h"
#include "MessageSystem.h"
#include "Pickable.h"
#include "RandomDice.h"
#include "Vector2D.h"

namespace
{
// One entry the table can draw, and the rule for its weight at a dungeon level.
struct ItemType
{
	// The item's name, or an enhanced rule's category.
	std::string name{};
	// Weight at the first level.
	int baseWeight{ 0 };
	int levelMinimum{ 0 };
	// No maximum when 0.
	int levelMaximum{ 0 };
	// How much the weight grows per level past the first; negative shrinks it.
	float levelScaling{ 0.0f };
	// The spawn category it is drawn under.
	std::string category{};
	std::function<void(Vector2D, GameContext&)> createFunc{};
};

// Places one enhanced item drawn from a rule's pool at position.
void place_enhanced_item(const EnhancedItemSpawnRule& rule, Vector2D position, GameContext& ctx)
{
	const int poolIndex = ctx.dice->roll(0, static_cast<int>(rule.itemPool.size()) - 1);
	const std::string_view baseKey = rule.itemPool[poolIndex];
	const ItemEnhancement enhancement = (rule.enhancementCategory == EnhancedItemCategory::WEAPON)
		? ItemEnhancement::generate_weapon_enhancement()
		: ItemEnhancement::generate_armor_enhancement();
	[[maybe_unused]] const auto spawnItemResult = InventoryOperations::add_item(
		*ctx.floorInventory,
		ItemCreator::create_with_enhancement(baseKey, position, enhancement.prefix, enhancement.suffix, ctx));
	assert(spawnItemResult.has_value());
}

// The spawn table as the registry holds it now: every item with a spawn weight, in key
// order, then every enhanced spawn rule.
std::vector<ItemType> spawn_table(const ItemRegistry& items)
{
	std::vector<ItemType> table{};

	for (const std::string& key : items.get_all_keys())
	{
		const ItemParams& params = items.get_params(key);
		if (params.baseWeight <= 0)
		{
			continue;
		}

		// A floor pile of gold is rolled for the level rather than worth its record.
		auto place_gold = [](Vector2D pos, GameContext& ctx)
		{
			const int level = ctx.levelManager->get_dungeon_level();
			const int amount = ctx.dice->roll(level * 3, level * 10);
			[[maybe_unused]] const auto spawnItemResult = InventoryOperations::add_item(
				*ctx.floorInventory,
				ItemCreator::create_with_gold_amount(pos, amount, ctx));
			assert(spawnItemResult.has_value());
		};
		auto place_item = [key](Vector2D pos, GameContext& ctx)
		{
			[[maybe_unused]] const auto spawnItemResult = InventoryOperations::add_item(
				*ctx.floorInventory,
				ItemCreator::create(key, pos, ctx));
			assert(spawnItemResult.has_value());
		};

		table.push_back(
			{
				.name = std::string{ params.name },
				.baseWeight = params.baseWeight,
				.levelMinimum = params.levelMin,
				.levelMaximum = params.levelMax,
				.levelScaling = params.levelScaling,
				.category = std::string{ params.category },
				.createFunc = (key == "gold_coin")
					? std::function<void(Vector2D, GameContext&)>{ place_gold }
					: std::function<void(Vector2D, GameContext&)>{ place_item },
			});
	}

	for (const EnhancedItemSpawnRule& rule : items.get_enhanced_rules())
	{
		table.push_back(
			{
				.name = rule.category,
				.baseWeight = rule.baseWeight,
				.levelMinimum = rule.levelMin,
				.levelMaximum = rule.levelMax,
				.levelScaling = rule.levelScaling,
				.category = rule.category,
				.createFunc = [rule](Vector2D pos, GameContext& ctx)
				{
					place_enhanced_item(rule, pos, ctx);
				},
			});
	}

	return table;
}

// An entry's weight at a dungeon level: 0 outside its levels, otherwise its base weight
// scaled by level and never below 1.
int calculate_weight(const ItemType& item, int dungeonLevel)
{
	if (dungeonLevel < item.levelMinimum || (item.levelMaximum > 0 && dungeonLevel > item.levelMaximum))
	{
		return 0;
	}

	const float levelFactor = 1.0f + (item.levelScaling * (dungeonLevel - 1));
	const int weight = static_cast<int>(item.baseWeight * levelFactor);

	return std::max(1, weight);
}

// Draws one entry of candidates by weight and places it; false when none can appear.
bool place_one_of(const std::vector<const ItemType*>& candidates, Vector2D position, int dungeonLevel, GameContext& ctx)
{
	int totalWeight = 0;
	std::vector<int> weights;

	for (const ItemType* candidate : candidates)
	{
		const int weight = calculate_weight(*candidate, dungeonLevel);
		weights.push_back(weight);
		totalWeight += weight;
	}

	if (totalWeight <= 0)
	{
		return false;
	}

	const int roll = ctx.dice->roll(1, totalWeight);
	int runningTotal = 0;

	for (size_t index = 0; index < candidates.size(); ++index)
	{
		runningTotal += weights[index];
		if (roll <= runningTotal)
		{
			candidates[index]->createFunc(position, ctx);
			break;
		}
	}
	return true;
}
} // namespace

void ItemFactory::generate_treasure(Vector2D position, int dungeonLevel, int quality, GameContext& ctx)
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

	default:
	{
		itemCount = 1;
	}
	}

	// Boost dungeon level for item generation to get better items
	const int effectiveLevel = dungeonLevel + (quality - 1) * 2;

	// Always include gold with amount based on quality
	const int goldMin = 10 * dungeonLevel * quality;
	const int goldMax = 20 * dungeonLevel * quality;
	const int goldAmount = ctx.dice->roll(goldMin, goldMax);

	// Through the registry, so a treasure pile is the same item as a floor pile: one
	// number for what it pays and what it is worth, and a tile the item data owns.
	auto goldPile = ItemCreator::create_with_gold_amount(position, goldAmount, ctx);
	[[maybe_unused]] const auto placed = InventoryOperations::add_item(*ctx.floorInventory, std::move(goldPile));
	assert(placed.has_value());

	// Generate other random items
	for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex)
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
		const int roll = ctx.dice->d100();

		if (quality == 3 && roll <= 5)
		{
			// 5% chance of very special items in exceptional quality treasure
			if (effectiveLevel >= 8 && ctx.dice->d100() <= 10)
			{
				// 10% chance for Amulet at high enough level
				spawn_item_of_category(itemPos, effectiveLevel, "artifact", ctx);
			}
			else
			{
				// High quality weapons
				spawn_item_of_category(itemPos, effectiveLevel + 2, "weapon", ctx);
			}
		}
		else if (roll <= 25)
		{
			// 25% chance of weapon
			spawn_item_of_category(itemPos, effectiveLevel, "weapon", ctx);
		}
		else if (roll <= 50)
		{
			// 25% chance of scroll
			spawn_item_of_category(itemPos, effectiveLevel, "scroll", ctx);
		}
		else if (roll <= 75)
		{
			// 25% chance of potion
			spawn_item_of_category(itemPos, effectiveLevel, "potion", ctx);
		}
		else
		{
			// 25% chance of food
			spawn_item_of_category(itemPos, effectiveLevel, "food", ctx);
		}
	}

	ctx.messageSystem->log("Generated treasure of quality " + std::to_string(quality) +
		" with " + std::to_string(itemCount + 1) + " items including gold");
}

std::vector<ItemPercentage> ItemFactory::get_current_distribution(int dungeonLevel, const ItemRegistry& items)
{
	const std::vector<ItemType> table = spawn_table(items);
	std::vector<ItemPercentage> distribution;

	int totalWeight = 0;
	std::vector<int> weights;

	for (const ItemType& item : table)
	{
		const int weight = calculate_weight(item, dungeonLevel);
		weights.push_back(weight);
		totalWeight += weight;
	}

	for (size_t index = 0; index < table.size(); ++index)
	{
		if (weights[index] > 0)
		{
			const float percentage = static_cast<float>(weights[index]) / totalWeight * 100.0f;
			distribution.push_back(ItemPercentage{ table[index].name, table[index].category, percentage });
		}
	}

	return distribution;
}

void ItemFactory::spawn_item_of_category(Vector2D position, int dungeonLevel, const std::string& category, GameContext& ctx)
{
	const std::vector<ItemType> table = spawn_table(*ctx.itemRegistry);

	std::vector<const ItemType*> candidates;
	for (const ItemType& item : table)
	{
		if (item.category == category)
		{
			candidates.push_back(&item);
		}
	}

	if (candidates.empty())
	{
		ctx.messageSystem->log("No items found in category: " + category);
		return;
	}

	if (!place_one_of(candidates, position, dungeonLevel, ctx))
	{
		ctx.messageSystem->log("No valid items in category " + category + " for this dungeon level!");
	}
}

void ItemFactory::spawn_random_item(Vector2D position, int dungeonLevel, GameContext& ctx)
{
	const std::vector<ItemType> table = spawn_table(*ctx.itemRegistry);

	std::vector<const ItemType*> candidates;
	for (const ItemType& item : table)
	{
		candidates.push_back(&item);
	}

	if (!place_one_of(candidates, position, dungeonLevel, ctx))
	{
		ctx.messageSystem->log("No valid items for this dungeon level!");
	}
}

void ItemFactory::spawn_all_enhanced_items_debug(Vector2D position, GameContext& ctx)
{
	ctx.messageSystem->log("DEBUG: Spawning enhanced items from all rules");

	for (const EnhancedItemSpawnRule& rule : ctx.itemRegistry->get_enhanced_rules())
	{
		place_enhanced_item(rule, position, ctx);
	}

	ctx.messageSystem->message(WHITE_BLACK_PAIR, "DEBUG: Spawned enhanced items", true);
}
