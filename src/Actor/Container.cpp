#include <algorithm>
#include <cassert>
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

#include "../Persistent/Persistent.h"
#include "../Utils/Vector2D.h"
#include "Actor.h"
#include "Item.h"
#include "Container.h"

// Construction with proper initialization
Container::Container(size_t initialCapacity) noexcept
	: capacity(initialCapacity),
	  eventHandler(nullptr)
{
	inventory.reserve(initialCapacity);
}

// Modern add method with proper error handling
ContainerResult<bool> Container::add(std::unique_ptr<Item> item)
{
	if (!item)
	{
		return std::unexpected(ContainerError::INVALID_ITEM);
	}

	if (is_full())
	{
		fire_event(ContainerEvent::Type::INVENTORY_FULL, item.get());
		return std::unexpected(ContainerError::FULL);
	}

	const auto* itemPtr = item.get();
	inventory.push_back(std::move(item));

	fire_event(ContainerEvent::Type::ITEM_ADDED, itemPtr);
	return true;
}

ContainerResult<std::unique_ptr<Item>> Container::remove(const Item& item)
{
	auto is_null = [](const auto& stored) { return !stored; };
	assert(std::ranges::none_of(inventory, is_null));

	auto matches_item = [&item](const auto& stored) { return stored.get() == &item; };
	auto matches = inventory | std::views::filter(matches_item);

	if (std::ranges::empty(matches))
	{
		return std::unexpected(ContainerError::ITEM_NOT_FOUND);
	}

	auto removedItem = std::move(matches.front());
	optimize_storage();

	fire_event(ContainerEvent::Type::ITEM_REMOVED, removedItem.get());

	return std::move(removedItem);
}

ContainerResult<std::unique_ptr<Item>> Container::remove_at(size_t index)
{
	if (index >= inventory.size())
	{
		return std::unexpected(ContainerError::ITEM_NOT_FOUND);
	}

	auto removedItem = std::move(inventory[index]);
	inventory.erase(inventory.begin() + index);

	fire_event(ContainerEvent::Type::ITEM_REMOVED, removedItem.get());
	optimize_storage();

	return std::move(removedItem);
}

// Item access methods
Item* Container::get_item_at(size_t index) noexcept
{
	return index < inventory.size() ? inventory[index].get() : nullptr;
}

const Item* Container::get_item_at(size_t index) const noexcept
{
	return index < inventory.size() ? inventory[index].get() : nullptr;
}

// Capacity management
void Container::set_capacity(size_t newCapacity)
{
	if (newCapacity < inventory.size())
	{
		inventory.resize(newCapacity);
	}

	capacity = newCapacity;
	inventory.reserve(capacity);

	fire_event(ContainerEvent::Type::CAPACITY_CHANGED);
}

// Search operations
const Item* Container::find_item_by_name(std::string_view name) const noexcept
{
	auto is_null = [](const auto& item) { return !item; };
	assert(std::ranges::none_of(inventory, is_null));

	auto it = std::ranges::find_if(inventory,
		[name](const auto& item)
		{
			return item->actorData.name == name;
		});

	return it != inventory.end() ? it->get() : nullptr;
}

bool Container::contains_item(const Item& item) const noexcept
{
	return std::ranges::any_of(inventory,
		[&item](const auto& stored_item)
		{
			return stored_item.get() == &item;
		});
}

// Event system implementation
void Container::fire_event(ContainerEvent::Type type, const Item* item)
{
	if (eventHandler)
	{
		ContainerEvent event{
			.type = type,
			.item = item,
			.currentSize = inventory.size(),
			.capacity = capacity
		};
		eventHandler(event);
	}
}

// Persistence with proper error handling
void Container::load(const json& j)
{
	try
	{
		capacity = j.value("capacity", 0);

		inventory.clear();
		inventory.reserve(capacity);

		if (j.contains("inventory") && j["inventory"].is_array())
		{
			for (const auto& item_json : j["inventory"])
			{
				auto item = std::make_unique<Item>(
					Vector2D{ 0, 0 },
					ActorData{});
				item->load(item_json);
				inventory.push_back(std::move(item));
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Container::load error: " << e.what() << std::endl;
	}
}

void Container::save(json& j)
{
	try
	{
		j["capacity"] = capacity;
		j["inventory"] = json::array();

		for (const auto& item : inventory)
		{
			if (item)
			{
				json item_json;
				item->save(item_json);
				j["inventory"].push_back(item_json);
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Container::save error: " << e.what() << std::endl;
	}
}

// Debug utilities
void Container::print_container(std::span<std::unique_ptr<Actor>> container) const
{
	int i = 0;
	for (const auto& item : inventory)
	{
		if (item)
		{
			std::cout << item->actorData.name << i << " ";
			i++;
		}
	}
	std::cout << '\n';
}

std::string Container::get_debug_info() const
{
	return std::format(
		"Container{{items:{}, capacity:{}, weight:{}/{}, full:{}}}",
		get_item_count(),
		get_capacity(),
		get_total_weight(),
		get_max_weight(),
		is_full() ? "yes" : "no");
}

int Container::get_total_weight() const noexcept
{
	int total = 0;
	for (const auto& item : inventory)
	{
		if (!item) continue;
		// Clamp individual item weight to non-negative
		const int item_weight = std::max(0, item->enhancement.weight);
		total += item_weight;
	}
	return total;
}

// Private helper methods
void Container::optimize_storage()
{
	std::erase_if(inventory, [](const auto& item)
		{ return !item; });

	if (inventory.size() * 4 < inventory.capacity())
	{
		inventory.shrink_to_fit();
	}
}
