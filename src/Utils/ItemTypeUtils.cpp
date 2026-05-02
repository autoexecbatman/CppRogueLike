#include <algorithm>
#include <cassert>
#include <memory>
#include <ranges>
#include <vector>

#include "ItemTypeUtils.h"
#include "../Actor/Container.h"
#include "../Actor/Actor.h"

namespace ItemTypeUtils
{
    Item* find_item_by_id(std::vector<std::unique_ptr<Item>>& inventory, int uniqueId)
    {
        auto is_null = [](const auto& item) { return !item; };
        assert(std::ranges::none_of(inventory, is_null));

        auto it = std::ranges::find_if(inventory,
            [uniqueId](const std::unique_ptr<Item>& item)
            {
                return item->uniqueId == uniqueId;
            });

        return (it != inventory.end()) ? it->get() : nullptr;
    }

    const Item* find_item_by_id(const std::vector<std::unique_ptr<Item>>& inventory, int uniqueId)
    {
        auto is_null = [](const auto& item) { return !item; };
        assert(std::ranges::none_of(inventory, is_null));

        auto it = std::ranges::find_if(inventory,
            [uniqueId](const std::unique_ptr<Item>& item)
            {
                return item->uniqueId == uniqueId;
            });

        return (it != inventory.end()) ? it->get() : nullptr;
    }

    std::unique_ptr<Item> extract_item_by_id(Container& container, int uniqueId)
    {
        auto& inventory = container.get_inventory_mutable();

        auto is_null = [](const auto& item) { return !item; };
        assert(std::ranges::none_of(inventory, is_null));

        auto matches_id = [uniqueId](const auto& item) { return item->uniqueId == uniqueId; };
        auto matches = inventory | std::views::filter(matches_id);

        if (std::ranges::empty(matches))
        {
            return nullptr;
        }

        auto extracted = std::move(matches.front());
        std::erase_if(inventory, is_null);
        return extracted;
    }
}
