// ItemTypeUtils.cpp - Implementation of ID-based and type-safe item utilities
#include "ItemTypeUtils.h"
#include "../Actor/Container.h"
#include "../Actor/Actor.h"

namespace ItemTypeUtils
{
    Item* find_item_by_id(const std::vector<std::unique_ptr<Item>>& inventory, UniqueId::IdType id)
    {
        for (const auto& item : inventory)
        {
            if (item && item->uniqueId == id)
            {
                return item.get();
            }
        }
        return nullptr;
    }
    
    Item* find_item_by_id(std::vector<std::unique_ptr<Item>>& inventory, UniqueId::IdType id)
    {
        for (auto& item : inventory)
        {
            if (item && item->uniqueId == id)
            {
                return item.get();
            }
        }
        return nullptr;
    }
    
    std::unique_ptr<Item> extract_item_by_id(Container& container, UniqueId::IdType id)
    {
        auto& inventory = container.get_inventory_mutable();
        for (auto& item : inventory)
        {
            if (item && item->uniqueId == id)
            {
                auto extracted = std::move(item);
                std::erase_if(inventory, [](const auto& item) { return !item; });
                return extracted;
            }
        }
        return nullptr;
    }
    
    std::unique_ptr<Item> extract_specific_item(Container& container, Item* target_item)
    {
        if (!target_item) return nullptr;
        return extract_item_by_id(container, target_item->uniqueId);
    }
}
