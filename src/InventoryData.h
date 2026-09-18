#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <vector>

#include "Item.h"

// Error handling for inventory operations
enum class InventoryError
{
	FULL,
	ITEM_NOT_FOUND,
	INVALID_ITEM,
	CAPACITY_EXCEEDED // only possible from add_item_to_inventory (CreatureInventory)
};

template <typename T>
using InventoryResult = std::expected<T, InventoryError>;

// Event system for decoupled notification
struct InventoryEvent
{
	enum class Type
	{
		ITEM_ADDED,
		ITEM_REMOVED,
		INVENTORY_FULL,
		CAPACITY_CHANGED
	};
	Type type{ Type::ITEM_ADDED };
	const Item* item{ nullptr };
	size_t currentSize{ 0 };
	size_t capacity{ 0 };
};

using InventoryEventHandler = std::function<void(const InventoryEvent&)>;

// Items on the dungeon floor.
// No weight owner. No carry cap. Cleared on level transition.
struct FloorInventory
{
	std::vector<std::unique_ptr<Item>> items;
	size_t capacity{ 0 };
	InventoryEventHandler eventHandler{ nullptr };

	explicit FloorInventory(size_t initialCapacity)
		: capacity(initialCapacity)
	{
		items.reserve(initialCapacity);
	}

	FloorInventory(FloorInventory&&) noexcept = default;
	FloorInventory& operator=(FloorInventory&&) noexcept = default;
	~FloorInventory() = default;
	FloorInventory(const FloorInventory&) = delete;
	FloorInventory& operator=(const FloorInventory&) = delete;
};

// A creature's personal backpack.
// Weight-gated by owner STR via add_item_to_inventory.
// Persisted with the owning Creature.
struct CreatureInventory
{
	std::vector<std::unique_ptr<Item>> items;
	size_t capacity{ 0 };
	InventoryEventHandler eventHandler{ nullptr };

	explicit CreatureInventory(size_t initialCapacity)
		: capacity(initialCapacity)
	{
		items.reserve(initialCapacity);
	}

	CreatureInventory(CreatureInventory&&) noexcept = default;
	CreatureInventory& operator=(CreatureInventory&&) noexcept = default;
	~CreatureInventory() = default;
	CreatureInventory(const CreatureInventory&) = delete;
	CreatureInventory& operator=(const CreatureInventory&) = delete;
};
