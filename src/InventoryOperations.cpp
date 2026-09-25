#include <algorithm>
#include <cassert>
#include <cstdint>
#include <expected>
#include <memory>
#include <ranges>
#include <utility>

#include "Actor.h"
#include "Creature.h"
#include "DataManager.h"
#include "InventoryData.h"
#include "InventoryOperations.h"
#include "Item.h"

namespace InventoryOperations
{

// ===== CORE OPERATIONS =====

InventoryResult<bool> add_item(FloorInventory& inventory, std::unique_ptr<Item> item)
{
	if (!item)
	{
		return std::unexpected(InventoryError::INVALID_ITEM);
	}

	if (is_inventory_full(inventory))
	{
		fire_inventory_event(inventory, InventoryEvent::Type::INVENTORY_FULL, item.get());
		return std::unexpected(InventoryError::FULL);
	}

	const auto* itemPtr = item.get();
	inventory.items.push_back(std::move(item));

	fire_inventory_event(inventory, InventoryEvent::Type::ITEM_ADDED, itemPtr);
	return true;
}

InventoryResult<bool> add_item(CreatureInventory& inventory, std::unique_ptr<Item> item)
{
	if (!item)
	{
		return std::unexpected(InventoryError::INVALID_ITEM);
	}

	if (is_inventory_full(inventory))
	{
		fire_inventory_event(inventory, InventoryEvent::Type::INVENTORY_FULL, item.get());
		return std::unexpected(InventoryError::FULL);
	}

	const auto* itemPtr = item.get();
	inventory.items.push_back(std::move(item));

	fire_inventory_event(inventory, InventoryEvent::Type::ITEM_ADDED, itemPtr);
	return true;
}

InventoryResult<bool> add_item_to_inventory(
	CreatureInventory& inventory,
	std::unique_ptr<Item> item,
	const Creature& owner,
	const DataManager& dataManager
)
{
	if (!item)
	{
		return std::unexpected(InventoryError::INVALID_ITEM);
	}

	if (is_inventory_full(inventory))
	{
		fire_inventory_event(inventory, InventoryEvent::Type::INVENTORY_FULL, item.get());
		return std::unexpected(InventoryError::FULL);
	}

	if (!is_within_weight_limit(*item, owner, dataManager))
	{
		fire_inventory_event(inventory, InventoryEvent::Type::INVENTORY_FULL, item.get());
		return std::unexpected(InventoryError::CAPACITY_EXCEEDED);
	}

	const auto* itemPtr = item.get();
	inventory.items.push_back(std::move(item));

	fire_inventory_event(inventory, InventoryEvent::Type::ITEM_ADDED, itemPtr);
	return true;
}

InventoryResult<std::unique_ptr<Item>> remove_item(FloorInventory& inventory, const Item& item)
{
	[[maybe_unused]] auto is_null = [](const auto& stored) { return !stored; };
	assert(std::ranges::none_of(inventory.items, is_null));

	auto matches_item = [&item](const auto& stored) { return stored.get() == &item; };
	auto matches = inventory.items | std::views::filter(matches_item);

	if (std::ranges::empty(matches))
	{
		return std::unexpected(InventoryError::ITEM_NOT_FOUND);
	}

	auto removedItem = std::move(matches.front());
	optimize_inventory_storage(inventory);

	fire_inventory_event(inventory, InventoryEvent::Type::ITEM_REMOVED, removedItem.get());
	return std::move(removedItem);
}

InventoryResult<std::unique_ptr<Item>> remove_item(CreatureInventory& inventory, const Item& item)
{
	[[maybe_unused]] auto is_null = [](const auto& stored) { return !stored; };
	assert(std::ranges::none_of(inventory.items, is_null));

	auto matches_item = [&item](const auto& stored) { return stored.get() == &item; };
	auto matches = inventory.items | std::views::filter(matches_item);

	if (std::ranges::empty(matches))
	{
		return std::unexpected(InventoryError::ITEM_NOT_FOUND);
	}

	auto removedItem = std::move(matches.front());
	optimize_inventory_storage(inventory);

	fire_inventory_event(inventory, InventoryEvent::Type::ITEM_REMOVED, removedItem.get());
	return std::move(removedItem);
}

InventoryResult<std::unique_ptr<Item>> remove_item_at(FloorInventory& inventory, size_t index)
{
	if (index >= inventory.items.size())
	{
		return std::unexpected(InventoryError::ITEM_NOT_FOUND);
	}

	auto removedItem = std::move(inventory.items[index]);
	inventory.items.erase(inventory.items.begin() + index);

	fire_inventory_event(inventory, InventoryEvent::Type::ITEM_REMOVED, removedItem.get());
	optimize_inventory_storage(inventory);

	return std::move(removedItem);
}

InventoryResult<std::unique_ptr<Item>> remove_item_at(CreatureInventory& inventory, size_t index)
{
	if (index >= inventory.items.size())
	{
		return std::unexpected(InventoryError::ITEM_NOT_FOUND);
	}

	auto removedItem = std::move(inventory.items[index]);
	inventory.items.erase(inventory.items.begin() + index);

	fire_inventory_event(inventory, InventoryEvent::Type::ITEM_REMOVED, removedItem.get());
	optimize_inventory_storage(inventory);

	return std::move(removedItem);
}

InventoryResult<std::unique_ptr<Item>> remove_item_by_id(CreatureInventory& inventory, uint64_t uniqueId)
{
	[[maybe_unused]] auto is_null = [](const auto& item) { return !item; };
	assert(std::ranges::none_of(inventory.items, is_null));

	auto matches_id = [uniqueId](const auto& item) { return item->uniqueId == uniqueId; };
	auto matches = inventory.items | std::views::filter(matches_id);

	if (std::ranges::empty(matches))
	{
		return std::unexpected(InventoryError::ITEM_NOT_FOUND);
	}

	auto removedItem = std::move(matches.front());
	optimize_inventory_storage(inventory);

	fire_inventory_event(inventory, InventoryEvent::Type::ITEM_REMOVED, removedItem.get());
	return std::move(removedItem);
}

// ===== CAPACITY MANAGEMENT =====

namespace
{
// What the book allows for the clothes a character stands up in.
constexpr int CLOTHING_POUNDS = 5;
} // namespace

int get_total_weight(const Creature& owner) noexcept
{
	// "Add five pounds for clothing, if any is worn" (Player's Handbook, PDF page 160).
	// Table 47 is Character Encumbrance and nothing here dresses a creature, so a
	// character starts from the allowance and a monster from nothing.
	int total = (owner.get_creature_class() == CreatureClass::MONSTER) ? 0 : CLOTHING_POUNDS;
	for (const auto& item : owner.inventoryData.items)
	{
		assert(item && "the pack owns every entry it holds");
		total += item->enhancement.weight;
	}

	// Worn gear is carried gear; only where it sits differs.
	for (const EquippedItem& equipped : owner.equippedItems)
	{
		assert(equipped.item && "an equipment slot owns the item in it");
		total += equipped.item->enhancement.weight;
	}
	return total;
}

int get_max_weight(const Creature& owner, const DataManager& dataManager) noexcept
{
	const StrengthAttributes row = dataManager.strength_for(owner.get_strength(), owner.get_exceptional_strength());
	return row.maxCarried;
}

bool is_overloaded(const Creature& owner, const DataManager& dataManager) noexcept
{
	return get_total_weight(owner) > get_max_weight(owner, dataManager);
}

bool is_within_weight_limit(
	const Item& item,
	const Creature& owner,
	const DataManager& dataManager
) noexcept
{
	return get_total_weight(owner) + item.enhancement.weight <= get_max_weight(owner, dataManager);
}

// ===== SEARCH OPERATIONS =====

Item* find_item_by_id(CreatureInventory& inventory, uint64_t uniqueId) noexcept
{
	[[maybe_unused]] auto is_null = [](const auto& item) { return !item; };
	assert(std::ranges::none_of(inventory.items, is_null));

	auto it = std::ranges::find_if(inventory.items,
		[uniqueId](const auto& item)
		{
			return item->uniqueId == uniqueId;
		});

	return it != inventory.items.end() ? it->get() : nullptr;
}

const Item* find_item_by_id(const CreatureInventory& inventory, uint64_t uniqueId) noexcept
{
	[[maybe_unused]] auto is_null = [](const auto& item) { return !item; };
	assert(std::ranges::none_of(inventory.items, is_null));

	auto it = std::ranges::find_if(inventory.items,
		[uniqueId](const auto& item)
		{
			return item->uniqueId == uniqueId;
		});

	return it != inventory.items.end() ? it->get() : nullptr;
}

} // namespace InventoryOperations
