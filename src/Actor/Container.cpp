// Container.cpp - Modern C++20 implementation with proper encapsulation
#include "Container.h"
#include "Actor.h"
#include "../Items/Items.h"
#include <iostream>
#include <algorithm>
#include <ranges>

// Construction with proper initialization
Container::Container(size_t initial_capacity) noexcept
    : capacity_(initial_capacity)
    , event_handler_(nullptr)
{
    inventory_.reserve(initial_capacity);
}

// Modern add method with proper error handling
ContainerResult<bool> Container::add(std::unique_ptr<Item> item)
{
    if (!item)
    {
        return ContainerError::INVALID_ITEM;
    }

    if (is_full())
    {
        fire_event(ContainerEvent::Type::INVENTORY_FULL, item.get());
        return ContainerError::FULL;
    }

    const auto* item_ptr = item.get();
    inventory_.push_back(std::move(item));
    
    fire_event(ContainerEvent::Type::ITEM_ADDED, item_ptr);
    return true;
}

ContainerResult<std::unique_ptr<Item>> Container::remove(const Item& item)
{
    auto it = std::ranges::find_if(inventory_, 
        [&item](const auto& stored_item) 
        { 
            return stored_item.get() == &item; 
        });

    if (it == inventory_.end())
    {
        return ContainerError::ITEM_NOT_FOUND;
    }

    auto removed_item = std::move(*it);
    inventory_.erase(it);
    
    fire_event(ContainerEvent::Type::ITEM_REMOVED, removed_item.get());
    optimize_storage();
    
    return std::move(removed_item);
}

ContainerResult<std::unique_ptr<Item>> Container::remove_at(size_t index)
{
    if (index >= inventory_.size())
    {
        return ContainerError::ITEM_NOT_FOUND;
    }

    auto removed_item = std::move(inventory_[index]);
    inventory_.erase(inventory_.begin() + index);
    
    fire_event(ContainerEvent::Type::ITEM_REMOVED, removed_item.get());
    optimize_storage();
    
    return std::move(removed_item);
}

// Legacy remove pattern (unusual but preserved for compatibility)
void Container::remove_legacy(std::unique_ptr<Item> actor)
{
    inventory_.push_back(std::move(actor));
    auto is_null = [](const auto& a) noexcept { return !a; };
    std::erase_if(inventory_, is_null);
}

// Item access methods
Item* Container::get_item_at(size_t index) noexcept
{
    return index < inventory_.size() ? inventory_[index].get() : nullptr;
}

const Item* Container::get_item_at(size_t index) const noexcept
{
    return index < inventory_.size() ? inventory_[index].get() : nullptr;
}

// Capacity management
void Container::set_capacity(size_t new_capacity)
{
    if (new_capacity < inventory_.size())
    {
        inventory_.resize(new_capacity);
    }
    
    capacity_ = new_capacity;
    inventory_.reserve(capacity_);
    
    fire_event(ContainerEvent::Type::CAPACITY_CHANGED);
}

// Search operations
const Item* Container::find_item_by_name(std::string_view name) const noexcept
{
    auto it = std::ranges::find_if(inventory_, 
        [name](const auto& item) 
        { 
            return item && item->actorData.name == name; 
        });
    
    return it != inventory_.end() ? it->get() : nullptr;
}

bool Container::contains_item(const Item& item) const noexcept
{
    return std::ranges::any_of(inventory_, 
        [&item](const auto& stored_item) 
        { 
            return stored_item.get() == &item; 
        });
}

// Event system implementation
void Container::fire_event(ContainerEvent::Type type, const Item* item)
{
    if (event_handler_)
    {
        ContainerEvent event
        {
            .type = type,
            .item = item,
            .current_size = inventory_.size(),
            .capacity = capacity_
        };
        event_handler_(event);
    }
}

// Persistence with proper error handling
void Container::load(const json& j)
{
    try
    {
        capacity_ = j.value("capacity", 0);
        
        inventory_.clear();
        inventory_.reserve(capacity_);
        
        if (j.contains("inventory") && j["inventory"].is_array())
        {
            for (const auto& item_json : j["inventory"])
            {
                auto item = std::make_unique<Item>(
                    Vector2D{0, 0}, 
                    ActorData{}
                );
                item->load(item_json);
                inventory_.push_back(std::move(item));
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
        j["capacity"] = capacity_;
        j["inventory"] = json::array();
        
        for (const auto& item : inventory_)
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
    for (const auto& item : inventory_)
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
    return "Container{items:" + std::to_string(get_item_count()) + 
           ", capacity:" + std::to_string(get_capacity()) + 
           ", full:" + (is_full() ? "yes" : "no") + "}";
}

// Private helper methods
void Container::optimize_storage()
{
    std::erase_if(inventory_, [](const auto& item) { return !item; });
    
    if (inventory_.size() * 4 < inventory_.capacity())
    {
        inventory_.shrink_to_fit();
    }
}
