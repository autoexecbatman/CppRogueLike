// InventoryOperations.cpp - Free function implementations
#include "InventoryOperations.h"
#include "InventoryData.h"
#include "Actor.h"
#include "../Items/Items.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <algorithm>
#include <ranges>

namespace InventoryOperations
{

// ===== CORE OPERATIONS =====

InventoryResult<bool> add_item(InventoryData& inventory, std::unique_ptr<Item> item)
{
    if (!item)
    {
        return InventoryError::INVALID_ITEM;
    }

    if (is_inventory_full(inventory))
    {
        fire_inventory_event(inventory, InventoryEvent::Type::INVENTORY_FULL, item.get());
        return InventoryError::FULL;
    }

    const auto* item_ptr = item.get();
    inventory.items.push_back(std::move(item));
    
    fire_inventory_event(inventory, InventoryEvent::Type::ITEM_ADDED, item_ptr);
    return true;
}

InventoryResult<std::unique_ptr<Item>> remove_item(InventoryData& inventory, const Item& item)
{
    auto it = std::ranges::find_if(inventory.items, 
        [&item](const auto& stored_item) 
        { 
            return stored_item.get() == &item; 
        });

    if (it == inventory.items.end())
    {
        return InventoryError::ITEM_NOT_FOUND;
    }

    auto removed_item = std::move(*it);
    inventory.items.erase(it);
    
    fire_inventory_event(inventory, InventoryEvent::Type::ITEM_REMOVED, removed_item.get());
    optimize_inventory_storage(inventory);
    
    return std::move(removed_item);
}

InventoryResult<std::unique_ptr<Item>> remove_item_at(InventoryData& inventory, size_t index)
{
    if (index >= inventory.items.size())
    {
        return InventoryError::ITEM_NOT_FOUND;
    }

    auto removed_item = std::move(inventory.items[index]);
    inventory.items.erase(inventory.items.begin() + index);
    
    fire_inventory_event(inventory, InventoryEvent::Type::ITEM_REMOVED, removed_item.get());
    optimize_inventory_storage(inventory);
    
    return std::move(removed_item);
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

void set_inventory_capacity(InventoryData& inventory, size_t new_capacity)
{
    if (new_capacity < inventory.items.size())
    {
        inventory.items.resize(new_capacity);
    }
    
    inventory.capacity = new_capacity;
    inventory.items.reserve(inventory.capacity);
    
    fire_inventory_event(inventory, InventoryEvent::Type::CAPACITY_CHANGED);
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
    auto it = std::ranges::find_if(inventory.items, 
        [name](const auto& item) 
        { 
            return item && item->actorData.name == name; 
        });
    
    return it != inventory.items.end() ? it->get() : nullptr;
}

Item* find_item_by_id(InventoryData& inventory, uint64_t unique_id) noexcept
{
    auto it = std::ranges::find_if(inventory.items,
        [unique_id](const auto& item)
        {
            return item && item->uniqueId == unique_id;
        });
    
    return it != inventory.items.end() ? it->get() : nullptr;
}

const Item* find_item_by_id(const InventoryData& inventory, uint64_t unique_id) noexcept
{
    auto it = std::ranges::find_if(inventory.items,
        [unique_id](const auto& item)
        {
            return item && item->uniqueId == unique_id;
        });
    
    return it != inventory.items.end() ? it->get() : nullptr;
}

InventoryResult<std::unique_ptr<Item>> remove_item_by_id(InventoryData& inventory, uint64_t unique_id)
{
    auto it = std::ranges::find_if(inventory.items,
        [unique_id](const auto& item)
        {
            return item && item->uniqueId == unique_id;
        });

    if (it == inventory.items.end())
    {
        return InventoryError::ITEM_NOT_FOUND;
    }

    auto removed_item = std::move(*it);
    inventory.items.erase(it);
    
    fire_inventory_event(inventory, InventoryEvent::Type::ITEM_REMOVED, removed_item.get());
    optimize_inventory_storage(inventory);
    
    return std::move(removed_item);
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
    inventory.event_handler = std::move(handler);
}

void fire_inventory_event(InventoryData& inventory, InventoryEvent::Type type, const Item* item)
{
    if (inventory.event_handler)
    {
        InventoryEvent event
        {
            .type = type,
            .item = item,
            .current_size = inventory.items.size(),
            .capacity = inventory.capacity
        };
        inventory.event_handler(event);
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
            for (const auto& item_json : j["inventory"])
            {
                auto item = std::make_unique<Item>(
                    Vector2D{0, 0}, 
                    ActorData{}
                );
                item->load(item_json);
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
                json item_json;
                item->save(item_json);
                j["inventory"].push_back(item_json);
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
    return "Inventory{items:" + std::to_string(get_item_count(inventory)) + 
           ", capacity:" + std::to_string(inventory.capacity) + 
           ", full:" + (is_inventory_full(inventory) ? "yes" : "no") + "}";
}

// ===== OPTIMIZATION =====

void optimize_inventory_storage(InventoryData& inventory)
{
    std::erase_if(inventory.items, [](const auto& item) { return !item; });
    
    if (inventory.items.size() * 4 < inventory.items.capacity())
    {
        inventory.items.shrink_to_fit();
    }
}

} // namespace InventoryOperations
