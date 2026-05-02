#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <vector>

#include "Item.h"

// - Data structures for inventory management

// Error handling for inventory operations
enum class InventoryError
{
	FULL,
	ITEM_NOT_FOUND,
	INVALID_ITEM,
	CAPACITY_EXCEEDED
};

template <typename T>
using InventoryResult = std::expected<T, InventoryError>;

// Event system for decoupled communication
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

// Pure data structure - no behavior
struct InventoryData
{
	std::vector<std::unique_ptr<Item>> items;
	size_t capacity{ 0 };
	InventoryEventHandler eventHandler{ nullptr };

	// Simple constructor
	explicit InventoryData(size_t initialCapacity)
		: capacity(initialCapacity), eventHandler(nullptr)
	{
		items.reserve(initialCapacity);
	}

	// Move semantics
	InventoryData(InventoryData&&) noexcept = default;
	InventoryData& operator=(InventoryData&&) noexcept = default;

	~InventoryData() = default;

	// No copying - unique ownership
	InventoryData(const InventoryData&) = delete;
	InventoryData& operator=(const InventoryData&) = delete;
};
