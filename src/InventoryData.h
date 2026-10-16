#pragma once

#include <expected>
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

// Items on the dungeon floor.
// No weight owner and no limit: an add always lands, so nothing dropped is ever lost.
// Cleared on level transition, which is the only bound on what it holds.
struct FloorInventory
{
	std::vector<std::unique_ptr<Item>> items{};

	FloorInventory() = default;
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
