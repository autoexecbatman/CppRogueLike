// ItemTypeUtils.h - Type-safe and ID-based item identification utilities
#ifndef ITEM_TYPE_UTILS_H
#define ITEM_TYPE_UTILS_H

#pragma once

#include <vector>
#include <memory>
#include <type_traits>
#include "UniqueId.h"

// Forward declarations
class Item;
class Container;

namespace ItemTypeUtils
{
    // ID-based item finding (preferred method)
    Item* find_item_by_id(const std::vector<std::unique_ptr<Item>>& inventory, UniqueId::IdType id);
    Item* find_item_by_id(std::vector<std::unique_ptr<Item>>& inventory, UniqueId::IdType id);
    
    // ID-based item extraction
    std::unique_ptr<Item> extract_item_by_id(Container& container, UniqueId::IdType id);
    
    // Extract specific item from container (legacy method)
    std::unique_ptr<Item> extract_specific_item(Container& container, Item* target_item);

    // Helper to get item ID safely
    UniqueId::IdType get_item_id(const Item* item);

    // Count items of specific type
    template<typename T>
    size_t count_items_of_type(const std::vector<std::unique_ptr<Item>>& inventory);
    
    // Type-safe item finding (for when you need the first item of a specific type)
    template<typename T>
    Item* find_item_of_type(const std::vector<std::unique_ptr<Item>>& inventory);

    template<typename T>
    Item* find_item_of_type(std::vector<std::unique_ptr<Item>>& inventory);

    // Type-safe item extraction
    template<typename T>
    std::unique_ptr<Item> extract_item_of_type(Container& container);

    // Type checking utilities
    template<typename T>
    bool is_item_of_type(const Item& item);
}

// Template implementations - must be in header for template instantiation
#include "../Actor/Actor.h" // For Item definition
#include "../Actor/Container.h" // For Container definition

namespace ItemTypeUtils
{
    template<typename T>
    size_t count_items_of_type(const std::vector<std::unique_ptr<Item>>& inventory)
    {
        size_t count = 0;
        for (const auto& item : inventory)
        {
            if (item && item->pickable && dynamic_cast<T*>(item->pickable.get()))
            {
                count++;
            }
        }
        return count;
    }
    
    template<typename T>
    Item* find_item_of_type(const std::vector<std::unique_ptr<Item>>& inventory)
    {
        for (const auto& item : inventory)
        {
            if (item && item->pickable && dynamic_cast<T*>(item->pickable.get()))
            {
                return item.get();
            }
        }
        return nullptr;
    }

    template<typename T>
    Item* find_item_of_type(std::vector<std::unique_ptr<Item>>& inventory)
    {
        for (auto& item : inventory)
        {
            if (item && item->pickable && dynamic_cast<T*>(item->pickable.get()))
            {
                return item.get();
            }
        }
        return nullptr;
    }

    template<typename T>
    std::unique_ptr<Item> extract_item_of_type(Container& container)
    {
        auto& inventory = container.get_inventory_mutable();
        for (auto& item : inventory)
        {
            if (item && item->pickable && dynamic_cast<T*>(item->pickable.get()))
            {
                auto extracted = std::move(item);
                std::erase_if(inventory, [](const auto& item) { return !item; });
                return extracted;
            }
        }
        return nullptr;
    }

    template<typename T>
    bool is_item_of_type(const Item& item)
    {
        return item.pickable && dynamic_cast<const T*>(item.pickable.get()) != nullptr;
    }

    inline UniqueId::IdType get_item_id(const Item* item)
    {
        return item ? item->uniqueId : UniqueId::INVALID_ID;
    }
}

#endif // ITEM_TYPE_UTILS_H
