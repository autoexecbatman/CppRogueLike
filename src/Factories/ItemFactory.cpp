#include <algorithm>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "../Actor/InventoryOperations.h"
#include "../Actor/Item.h"
#include "../Actor/Pickable.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Systems/ContentRegistry.h"
#include "../Systems/TileConfig.h"
#include "../Items/ItemClassification.h"
#include "../Map/Map.h"
#include "../Random/RandomDice.h"
#include "../Systems/ItemEnhancements/ItemEnhancements.h"
#include "../Systems/LevelManager.h"
#include "../Systems/MessageSystem.h"
#include "../Utils/Vector2D.h"
#include "ItemCreator.h"
#include "ItemFactory.h"

using namespace InventoryOperations; // For clean function calls

ItemFactory::ItemFactory()
{
	load_from_registry();
	load_enhanced_rules(ItemCreator::get_enhanced_rules());

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

void ItemFactory::load_from_registry()
{
	for (const auto& key : ItemCreator::get_all_keys())
	{
		const ItemParams& params = ItemCreator::get_params(key);
		if (params.baseWeight <= 0)
			continue;

		std::string capturedKey = key;
		auto createFunc = (capturedKey == "gold_coin")
			? std::function<void(Vector2D, GameContext&)>{
				  [](Vector2D pos, GameContext& ctx)
				  {
					  const int level = ctx.levelManager->get_dungeon_level();
					  const int amount = ctx.dice->roll(level * 3, level * 10);
					  InventoryOperations::add_item(
						  *ctx.inventoryData,
						  ItemCreator::create_with_gold_amount(pos, amount, *ctx.contentRegistry));
				  }
			  }
			: std::function<void(Vector2D, GameContext&)>{ [capturedKey](Vector2D pos, GameContext& ctx)
				  {
					  InventoryOperations::add_item(
						  *ctx.inventoryData,
						  ItemCreator::create(capturedKey, pos, *ctx.contentRegistry));
				  } };

		add_item_type(
			{
				std::string{ params.name },
				params.baseWeight,
				params.levelMin,
				params.levelMax,
				params.levelScaling,
				std::string{ params.category },
				std::move(createFunc)
			});
	}
}

void ItemFactory::load_enhanced_rules(std::span<const EnhancedItemSpawnRule> rules)
{
	for (const auto& rule : rules)
	{
		add_item_type(
			{
				rule.category,
				rule.baseWeight,
				rule.levelMin,
				rule.levelMax,
				rule.levelScaling,
				rule.category,
				[rule](Vector2D pos, GameContext& ctx)
				{
					const int idx = ctx.dice->roll(0, static_cast<int>(rule.itemPool.size()) - 1);
					const std::string_view baseKey = rule.itemPool[idx];
					if (rule.enhancementCategory == EnhancedItemCategory::WEAPON)
					{
						auto enh = ItemEnhancement::generate_weapon_enhancement();
						InventoryOperations::add_item(
							*ctx.inventoryData,
							ItemCreator::create_with_enhancement(
								baseKey, pos, enh.prefix, enh.suffix, *ctx.contentRegistry));
					}
					else
					{
						auto enh = ItemEnhancement::generate_armor_enhancement();
						InventoryOperations::add_item(
							*ctx.inventoryData,
							ItemCreator::create_with_enhancement(
								baseKey, pos, enh.prefix, enh.suffix, *ctx.contentRegistry));
					}
				}
			});
	}
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

	default:
	{
		itemCount = 1;
	}
	}

	// Boost dungeon level for item generation to get better items
	int effectiveLevel = dungeonLevel + (quality - 1) * 2;

	// Always include gold with amount based on quality
	int goldMin = 10 * dungeonLevel * quality;
	int goldMax = 20 * dungeonLevel * quality;
	int goldAmount = ctx.dice->roll(goldMin, goldMax);

	// Create gold pile
	auto goldPile = std::make_unique<Item>(position, ActorData{ ctx.tileConfig->get("TILE_GOLD"), "gold pile", YELLOW_BLACK_PAIR });
	goldPile->behavior = Gold{ goldAmount };
	add_item(*ctx.inventoryData, std::move(goldPile));

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

	ctx.messageSystem->log("Generated treasure of quality " + std::to_string(quality) +
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
			distribution.push_back(ItemPercentage{ itemTypes[i].name, itemTypes[i].category, percentage });
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
		ctx.messageSystem->log("No items found in category: " + category);
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
		ctx.messageSystem->log("No valid items in category " + category + " for this dungeon level!");
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
		ctx.messageSystem->log("No valid items for this dungeon level!");
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
	if (dungeonLevel < item.levelMinimum || (item.levelMaximum > 0 && dungeonLevel > item.levelMaximum))
	{
		return 0;
	}

	// Calculate scaled weight based on dungeon level
	float levelFactor = 1.0f + (item.levelScaling * (dungeonLevel - 1));
	int weight = static_cast<int>(item.baseWeight * levelFactor);

	// Ensure weight is at least 1 if item is available at this level
	return std::max(1, weight);
}

void ItemFactory::spawn_all_enhanced_items_debug(Vector2D position, GameContext& ctx)
{
	ctx.messageSystem->log("DEBUG: Spawning enhanced items from all rules");

	auto spawn_rule_sample = [&](std::span<const EnhancedItemSpawnRule> rules)
	{
		for (const auto& rule : rules)
		{
			const int idx = ctx.dice->roll(
				0, static_cast<int>(rule.itemPool.size()) - 1);
			const std::string_view baseKey = rule.itemPool[idx];
			if (rule.enhancementCategory == EnhancedItemCategory::WEAPON)
			{
				auto enh = ItemEnhancement::generate_weapon_enhancement();
				InventoryOperations::add_item(
					*ctx.inventoryData,
					ItemCreator::create_with_enhancement(baseKey, position, enh.prefix, enh.suffix, *ctx.contentRegistry));
			}
			else
			{
				auto enh = ItemEnhancement::generate_armor_enhancement();
				InventoryOperations::add_item(
					*ctx.inventoryData,
					ItemCreator::create_with_enhancement(baseKey, position, enh.prefix, enh.suffix, *ctx.contentRegistry));
			}
		}
	};

	spawn_rule_sample(ItemCreator::get_enhanced_rules());
	ctx.messageSystem->message(WHITE_BLACK_PAIR, "DEBUG: Spawned enhanced items", true);
}