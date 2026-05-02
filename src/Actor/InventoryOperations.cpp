#include <algorithm>
#include <cassert>
#include <cstdint>
#include <exception>
#include <expected>
#include <format>
#include <iostream>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "../Combat/WeightTier.h"
#include "../Utils/Vector2D.h"
#include "Actor.h"
#include "Creature.h"
#include "InventoryData.h"
#include "InventoryOperations.h"
#include "Item.h"

using json = nlohmann::json;

namespace InventoryOperations
{

// ===== CORE OPERATIONS =====

InventoryResult<bool> add_item(InventoryData& inventory, std::unique_ptr<Item> item)
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
	InventoryData& inventory,
	std::unique_ptr<Item> item,
	const Creature& owner
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

	// Check weight capacity: hard stop if total weight would exceed max
	const int itemWeight = item->enhancement.weight;
	const int currentWeight = get_total_weight(inventory);
	const int maxWeight = get_max_weight(owner);

	if (currentWeight + itemWeight > maxWeight)
	{
		fire_inventory_event(inventory, InventoryEvent::Type::INVENTORY_FULL, item.get());
		return std::unexpected(InventoryError::CAPACITY_EXCEEDED);
	}

	const auto* item_ptr = item.get();
	inventory.items.push_back(std::move(item));

	fire_inventory_event(inventory, InventoryEvent::Type::ITEM_ADDED, item_ptr);
	return true;
}

InventoryResult<std::unique_ptr<Item>> remove_item(InventoryData& inventory, const Item& item)
{
	auto is_null = [](const auto& stored) { return !stored; };
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

InventoryResult<std::unique_ptr<Item>> remove_item_at(InventoryData& inventory, size_t index)
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

// ===== CAPACITY MANAGEMENT =====

bool is_inventory_full(const InventoryData& inventory) noexcept
{
	return inventory.items.size() >= inventory.capacity;
}

bool is_inventory_empty(const InventoryData& inventory) noexcept
{
	return inventory.items.empty();
}

size_t get_item_count(const InventoryData& inventory) noexcept
{
	return inventory.items.size();
}

size_t get_remaining_space(const InventoryData& inventory) noexcept
{
	return inventory.capacity - inventory.items.size();
}

void set_inventory_capacity(InventoryData& inventory, size_t newCapacity)
{
	if (newCapacity < inventory.items.size())
	{
		inventory.items.resize(newCapacity);
	}

	inventory.capacity = newCapacity;
	inventory.items.reserve(inventory.capacity);

	fire_inventory_event(inventory, InventoryEvent::Type::CAPACITY_CHANGED);
}

int get_total_weight(const InventoryData& inventory) noexcept
{
	int total = 0;
	for (const auto& item : inventory.items)
	{
		if (item)
		{
			total += item->enhancement.weight;
		}
	}
	return total;
}

int get_max_weight(const Creature& owner) noexcept
{
	return calculate_max_weight(owner.get_strength());
}

bool is_overloaded(const InventoryData& inventory, const Creature& owner) noexcept
{
	return get_total_weight(inventory) > get_max_weight(owner);
}

// ===== ITEM ACCESS =====

Item* get_item_at(InventoryData& inventory, size_t index) noexcept
{
	return index < inventory.items.size() ? inventory.items[index].get() : nullptr;
}

const Item* get_item_at(const InventoryData& inventory, size_t index) noexcept
{
	return index < inventory.items.size() ? inventory.items[index].get() : nullptr;
}

// ===== SEARCH OPERATIONS =====

const Item* find_item_by_name(const InventoryData& inventory, std::string_view name) noexcept
{
	auto is_null = [](const auto& item) { return !item; };
	assert(std::ranges::none_of(inventory.items, is_null));

	auto it = std::ranges::find_if(inventory.items,
		[name](const auto& item)
		{
			return item->actorData.name == name;
		});

	return it != inventory.items.end() ? it->get() : nullptr;
}

Item* find_item_by_id(InventoryData& inventory, uint64_t uniqueId) noexcept
{
	auto is_null = [](const auto& item) { return !item; };
	assert(std::ranges::none_of(inventory.items, is_null));

	auto it = std::ranges::find_if(inventory.items,
		[uniqueId](const auto& item)
		{
			return item->uniqueId == uniqueId;
		});

	return it != inventory.items.end() ? it->get() : nullptr;
}

const Item* find_item_by_id(const InventoryData& inventory, uint64_t uniqueId) noexcept
{
	auto is_null = [](const auto& item) { return !item; };
	assert(std::ranges::none_of(inventory.items, is_null));

	auto it = std::ranges::find_if(inventory.items,
		[uniqueId](const auto& item)
		{
			return item->uniqueId == uniqueId;
		});

	return it != inventory.items.end() ? it->get() : nullptr;
}

InventoryResult<std::unique_ptr<Item>> remove_item_by_id(InventoryData& inventory, uint64_t uniqueId)
{
	auto is_null = [](const auto& item) { return !item; };
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

bool contains_item(const InventoryData& inventory, const Item& item) noexcept
{
	return std::ranges::any_of(inventory.items,
		[&item](const auto& stored_item)
		{
			return stored_item.get() == &item;
		});
}

// ===== EVENT SYSTEM =====

void set_inventory_event_handler(InventoryData& inventory, InventoryEventHandler handler)
{
	inventory.eventHandler = std::move(handler);
}

void fire_inventory_event(InventoryData& inventory, InventoryEvent::Type type, const Item* item)
{
	if (inventory.eventHandler)
	{
		InventoryEvent event{
			.type = type,
			.item = item,
			.currentSize = inventory.items.size(),
			.capacity = inventory.capacity
		};
		inventory.eventHandler(event);
	}
}

// ===== PERSISTENCE =====

void load_inventory(InventoryData& inventory, const json& j)
{
	try
	{
		inventory.capacity = j.value("capacity", 0);

		inventory.items.clear();
		inventory.items.reserve(inventory.capacity);

		if (j.contains("inventory") && j["inventory"].is_array())
		{
			for (const auto& itemJson : j["inventory"])
			{
				auto item = std::make_unique<Item>(
					Vector2D{ 0, 0 },
					ActorData{});
				item->load(itemJson);
				inventory.items.push_back(std::move(item));
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "load_inventory error: " << e.what() << std::endl;
	}
}

void save_inventory(const InventoryData& inventory, json& j)
{
	try
	{
		j["capacity"] = inventory.capacity;
		j["inventory"] = json::array();

		for (const auto& item : inventory.items)
		{
			if (item)
			{
				json itemJson;
				item->save(itemJson);
				j["inventory"].push_back(itemJson);
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "save_inventory error: " << e.what() << std::endl;
	}
}

// ===== DEBUG UTILITIES =====

void print_inventory(const InventoryData& inventory, std::span<std::unique_ptr<Actor>> actors)
{
	int i = 0;
	for (const auto& item : inventory.items)
	{
		if (item)
		{
			std::cout << item->actorData.name << i << " ";
			i++;
		}
	}
	std::cout << '\n';
}

std::string get_inventory_debug_info(const InventoryData& inventory)
{
	return std::format(
		"Inventory{{items:{}, capacity:{}, full:{}}}",
		get_item_count(inventory),
		inventory.capacity,
		is_inventory_full(inventory) ? "yes" : "no");
}

// ===== OPTIMIZATION =====

void optimize_inventory_storage(InventoryData& inventory)
{
	std::erase_if(inventory.items,
		[](const auto& item)
		{ return !item; });

	if (inventory.items.size() * 4 < inventory.items.capacity())
	{
		inventory.items.shrink_to_fit();
	}
}

} // namespace InventoryOperations
