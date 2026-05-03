#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
#include <ranges>
#include <span>
#include <string>

#include <nlohmann/json.hpp>

#include "../Utils/Vector2D.h"
#include "Actor.h"
#include "InventoryData.h"
#include "Item.h"

// Forward declarations
class Creature;

namespace InventoryOperations
{

// Both FloorInventory and CreatureInventory share the same data shape.
// This concept gates shared operations. Weight-gated add is CreatureInventory-only.
template <typename T>
concept AnyInventory = std::same_as<T, FloorInventory> || std::same_as<T, CreatureInventory>;

// ===== CORE OPERATIONS =====
// Non-template — defined in InventoryOperations.cpp

// Add to floor. No weight check. Use for loot spawns, drops, and corpses.
InventoryResult<bool> add_item(FloorInventory& inventory, std::unique_ptr<Item> item);

// Add to creature backpack. Capacity check only — no weight gate.
// Use for initialization (starting gear, load). Weight gate lives in add_item_to_inventory.
InventoryResult<bool> add_item(CreatureInventory& inventory, std::unique_ptr<Item> item);

// Add to creature backpack with STR-based weight enforcement.
InventoryResult<bool> add_item_to_inventory(
	CreatureInventory& inventory,
	std::unique_ptr<Item> item,
	const Creature& owner
);

// Remove from floor
InventoryResult<std::unique_ptr<Item>> remove_item(FloorInventory& inventory, const Item& item);
InventoryResult<std::unique_ptr<Item>> remove_item_at(FloorInventory& inventory, size_t index);

// Remove from creature backpack
InventoryResult<std::unique_ptr<Item>> remove_item(CreatureInventory& inventory, const Item& item);
InventoryResult<std::unique_ptr<Item>> remove_item_at(CreatureInventory& inventory, size_t index);

// Remove from creature backpack by id
InventoryResult<std::unique_ptr<Item>> remove_item_by_id(CreatureInventory& inventory, uint64_t uniqueId);

// Weight management — creature backpack only
int get_total_weight(const CreatureInventory& inventory) noexcept;
int get_max_weight(const Creature& owner) noexcept;
bool is_overloaded(const CreatureInventory& inventory, const Creature& owner) noexcept;

// id-based search — creature backpack only
Item* find_item_by_id(CreatureInventory& inventory, uint64_t uniqueId) noexcept;
const Item* find_item_by_id(const CreatureInventory& inventory, uint64_t uniqueId) noexcept;

// ===== CAPACITY MANAGEMENT =====

template <AnyInventory T>
bool is_inventory_full(const T& inventory) noexcept
{
	return inventory.items.size() >= inventory.capacity;
}

template <AnyInventory T>
bool is_inventory_empty(const T& inventory) noexcept
{
	return inventory.items.empty();
}

template <AnyInventory T>
size_t get_item_count(const T& inventory) noexcept
{
	return inventory.items.size();
}

template <AnyInventory T>
size_t get_remaining_space(const T& inventory) noexcept
{
	return inventory.capacity - inventory.items.size();
}

template <AnyInventory T>
void set_inventory_capacity(T& inventory, size_t newCapacity)
{
	if (newCapacity < inventory.items.size())
	{
		inventory.items.resize(newCapacity);
	}

	inventory.capacity = newCapacity;
	inventory.items.reserve(inventory.capacity);

	if (inventory.eventHandler)
	{
		InventoryEvent event{
			.type = InventoryEvent::Type::CAPACITY_CHANGED,
			.item = nullptr,
			.currentSize = inventory.items.size(),
			.capacity = inventory.capacity
		};
		inventory.eventHandler(event);
	}
}

// ===== ITEM ACCESS =====

template <AnyInventory T>
Item* get_item_at(T& inventory, size_t index) noexcept
{
	return index < inventory.items.size() ? inventory.items[index].get() : nullptr;
}

template <AnyInventory T>
const Item* get_item_at(const T& inventory, size_t index) noexcept
{
	return index < inventory.items.size() ? inventory.items[index].get() : nullptr;
}

// ===== SEARCH OPERATIONS =====

template <AnyInventory T>
const Item* find_item_by_name(const T& inventory, std::string_view name) noexcept
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

template <AnyInventory T>
bool contains_item(const T& inventory, const Item& item) noexcept
{
	return std::ranges::any_of(inventory.items,
		[&item](const auto& storedItem)
		{
			return storedItem.get() == &item;
		});
}

// ===== EVENT SYSTEM =====

template <AnyInventory T>
void set_inventory_event_handler(T& inventory, InventoryEventHandler handler)
{
	inventory.eventHandler = std::move(handler);
}

template <AnyInventory T>
void fire_inventory_event(T& inventory, InventoryEvent::Type type, const Item* item = nullptr)
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

template <AnyInventory T>
void load_inventory(T& inventory, const nlohmann::json& j)
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
				auto item = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{});
				item->load(itemJson);
				inventory.items.push_back(std::move(item));
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "load_inventory error: " << e.what() << '\n';
	}
}

template <AnyInventory T>
void save_inventory(const T& inventory, nlohmann::json& j)
{
	try
	{
		j["capacity"] = inventory.capacity;
		j["inventory"] = nlohmann::json::array();

		for (const auto& item : inventory.items)
		{
			if (item)
			{
				nlohmann::json itemJson;
				item->save(itemJson);
				j["inventory"].push_back(itemJson);
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "save_inventory error: " << e.what() << '\n';
	}
}

// ===== DEBUG =====

template <AnyInventory T>
std::string get_inventory_debug_info(const T& inventory)
{
	return std::format(
		"Inventory{{items:{}, capacity:{}, full:{}}}",
		get_item_count(inventory),
		inventory.capacity,
		is_inventory_full(inventory) ? "yes" : "no");
}

template <AnyInventory T>
void print_inventory(const T& inventory, std::span<std::unique_ptr<Actor>> /*actors*/)
{
	int i = 0;
	for (const auto& item : inventory.items)
	{
		if (item)
		{
			std::cout << item->actorData.name << i << " ";
			++i;
		}
	}
	std::cout << '\n';
}

// ===== OPTIMIZATION =====

template <AnyInventory T>
void optimize_inventory_storage(T& inventory)
{
	std::erase_if(inventory.items, [](const auto& item) { return !item; });

	if (inventory.items.size() * 4 < inventory.items.capacity())
	{
		inventory.items.shrink_to_fit();
	}
}

} // namespace InventoryOperations
