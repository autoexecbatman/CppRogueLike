#include <algorithm>

#include "ItemTypeUtils.h"
#include "../Actor/Container.h"

namespace ItemTypeUtils
{
    Item* find_item_by_id(std::vector<std::unique_ptr<Item>>& inventory, int uniqueId)
    {
        auto it = std::find_if(inventory.begin(), inventory.end(),
            [uniqueId](const std::unique_ptr<Item>& item) 
            {
                return item && item->uniqueId == uniqueId;
            });
        
        return (it != inventory.end()) ? it->get() : nullptr;
    }
    
    const Item* find_item_by_id(const std::vector<std::unique_ptr<Item>>& inventory, int uniqueId)
    {
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
