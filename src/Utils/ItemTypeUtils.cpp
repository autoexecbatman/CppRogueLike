#include <memory>
#include <vector>
#include <algorithm>

#include "ItemTypeUtils.h"
#include "../Actor/Container.h"
#include "../Actor/Actor.h"

namespace ItemTypeUtils
{
    Item* find_item_by_id(std::vector<std::unique_ptr<Item>>& inventory, int uniqueId)
    {
        // TODO: replace std::find_if with std::ranges::find_if + ranges view
        auto it = std::find_if(inventory.begin(), inventory.end(),
            [uniqueId](const std::unique_ptr<Item>& item)
            {
                return item && item->uniqueId == uniqueId;
            });

        return (it != inventory.end()) ? it->get() : nullptr;
    }

    const Item* find_item_by_id(const std::vector<std::unique_ptr<Item>>& inventory, int uniqueId)
    {
        // TODO: replace std::find_if with std::ranges::find_if + ranges view
        auto it = std::find_if(inventory.begin(), inventory.end(),
            [uniqueId](const std::unique_ptr<Item>& item)
            {
                return item && item->uniqueId == uniqueId;
            });

        return (it != inventory.end()) ? it->get() : nullptr;
    }

    std::unique_ptr<Item> extract_item_by_id(Container& container, int uniqueId)
    {
        auto& inventory = container.get_inventory_mutable();

        // TODO: replace std::find_if + erase with ranges (erase requires iterator for now)
        auto it = std::find_if(inventory.begin(), inventory.end(),
            [uniqueId](const std::unique_ptr<Item>& item)
            {
                return item && item->uniqueId == uniqueId;
            });

        if (it != inventory.end())
        {
            auto extracted_item = std::move(*it);
            inventory.erase(it);
            return extracted_item;
        }
        
        return nullptr;
    }
}
