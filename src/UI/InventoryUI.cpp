#include "InventoryUI.h"
#include "../Actor/Actor.h"
#include "../ActorTypes/Player.h"
#include "../Actor/Pickable.h"
#include "../Actor/InventoryOperations.h"
#include "../Game.h"
#include "../Controls/Controls.h"
#include "../Colors/Colors.h"
#include <algorithm>

using namespace InventoryOperations; // For clean function calls without namespace prefix

InventoryUI::InventoryUI() : inventoryWindow(nullptr), equipmentWindow(nullptr)
{
}

InventoryUI::~InventoryUI()
{
    destroy_windows();
}

void InventoryUI::display(Player& player, GameContext& ctx)
{
    while (true)
    {
        create_windows();
        
        // Display content - separated backpack and equipment
        display_inventory_items(player);
        display_equipment_slots(player);
        display_instructions(ctx);
        
        refresh_windows();
        
        // Handle input - returns false to exit inventory
        if (!handle_inventory_input(player, ctx))
        {
            break;
        }
        
        destroy_windows();
    }
    
    restore_game_display(ctx);
}

void InventoryUI::create_windows()
{
    clear();
    refresh();
    
    inventoryWindow = newwin(LINES, get_inventory_width(), 0, 0);
    equipmentWindow = newwin(LINES, get_equipment_width(), 0, get_inventory_width());
    
    box(inventoryWindow, 0, 0);
    box(equipmentWindow, 0, 0);
    
    // Window titles
    wattron(inventoryWindow, A_BOLD);
    mvwprintw(inventoryWindow, 0, 2, " BACKPACK ");
    wattroff(inventoryWindow, A_BOLD);
    
    wattron(equipmentWindow, A_BOLD);
    mvwprintw(equipmentWindow, 0, 2, " EQUIPMENT ");
    wattroff(equipmentWindow, A_BOLD);
}

void InventoryUI::destroy_windows()
{
    if (inventoryWindow)
    {
        delwin(inventoryWindow);
        inventoryWindow = nullptr;
    }
    if (equipmentWindow)
    {
        delwin(equipmentWindow);
        equipmentWindow = nullptr;
    }
}

void InventoryUI::refresh_windows()
{
    wrefresh(inventoryWindow);
    wrefresh(equipmentWindow);
}

std::vector<std::pair<Item*, bool>> InventoryUI::build_item_list(const Player& player)
{
    std::vector<std::pair<Item*, bool>> allItems;
    
    // Add all items from inventory with their equipped status
    for (const auto& item : player.inventory_data.items)
        {
            if (item)
            {
                bool isEquipped = false;
                
                // Check if item is equipped using proper slot-based system
                for (const auto& equippedItem : player.equippedItems)
                {
                    if (equippedItem.item && equippedItem.item->uniqueId == item->uniqueId)
                    {
                        isEquipped = true;
                        break;
                    }
                }
                
                allItems.emplace_back(item.get(), isEquipped);
            }
        }
    
    return allItems;
}

void InventoryUI::display_inventory_items(const Player& player)
{
    // Update title with backpack item count
    size_t backpackItems = InventoryOperations::get_item_count(player.inventory_data);
    size_t maxItems = player.inventory_data.capacity;
    mvwprintw(inventoryWindow, 0, 2, " BACKPACK (%zu/%zu) ", backpackItems, maxItems);
    
    int y = 2;
    char shortcut = 'a';
    
    // Build list of valid items (same logic as selection)
    std::vector<Item*> validItems;
    for (const auto& item : player.inventory_data.items)
    {
        if (item)
        {
            validItems.push_back(item.get());
        }
    }
    
    if (validItems.empty())
    {
        mvwprintw(inventoryWindow, y, 2, "Your backpack is empty.");
        return;
    }
    
    // Display valid items with consistent indexing
    for (size_t i = 0; i < validItems.size() && shortcut <= 'z'; i++)
    {
        Item* item = validItems[i];
        
        mvwprintw(inventoryWindow, y, 2, "%c) ", shortcut);
        show_item_info(item, y);
        
        y++;
        shortcut++;
    }
}

void InventoryUI::show_item_info(Item* item, int y)
{
    // Display item name with color
    wattron(inventoryWindow, COLOR_PAIR(item->actorData.color));
    wprintw(inventoryWindow, "%s", item->actorData.name.c_str());
    wattroff(inventoryWindow, COLOR_PAIR(item->actorData.color));
    
    // Show value if applicable
    if (item->value > 0)
    {
        wattron(inventoryWindow, COLOR_PAIR(YELLOW_BLACK_PAIR));
        wprintw(inventoryWindow, " (%d gp)", item->value);
        wattroff(inventoryWindow, COLOR_PAIR(YELLOW_BLACK_PAIR));
    }
    
    // Show weapon damage if applicable using Item's classification methods
    if (item->is_weapon())
    {
        Weapon* weapon = static_cast<Weapon*>(item->pickable.get()); // Safe after type check
        std::string damageInfo = " [" + weapon->roll + (weapon->is_ranged() ? " rng dmg]" : " dmg]");
        wattron(inventoryWindow, COLOR_PAIR(WHITE_BLACK_PAIR));
        wprintw(inventoryWindow, "%s", damageInfo.c_str());
        wattroff(inventoryWindow, COLOR_PAIR(WHITE_BLACK_PAIR));
    }
}

void InventoryUI::display_equipment_slots(const Player& player)
{
    int y = 2;

    // Display all equipment slots ADOM-style
    struct SlotInfo
    {
        EquipmentSlot slot;
        const char* name;
        char shortcut;
    };

    const SlotInfo slots[] = 
    {
        {EquipmentSlot::HEAD, "Head", 'H'},
        {EquipmentSlot::NECK, "Neck", 'N'},
        {EquipmentSlot::BODY, "Body", 'B'},
        {EquipmentSlot::GIRDLE, "Girdle", 'G'},
        {EquipmentSlot::CLOAK, "Cloak", 'C'},
        {EquipmentSlot::RIGHT_HAND, "Right Hand", 'R'},
        {EquipmentSlot::LEFT_HAND, "Left Hand", 'L'},
        {EquipmentSlot::RIGHT_RING, "Right Ring", ')'},
        {EquipmentSlot::LEFT_RING, "Left Ring", '('},
        {EquipmentSlot::BRACERS, "Bracers", '['},
        {EquipmentSlot::GAUNTLETS, "Gauntlets", ']'},
        {EquipmentSlot::BOOTS, "Boots", 'F'},
        {EquipmentSlot::MISSILE_WEAPON, "Missile Weapon", 'M'},
        {EquipmentSlot::MISSILES, "Missiles", 'A'},
        {EquipmentSlot::TOOL, "Tool", 'T'}
    };

    for (const auto& slotInfo : slots)
    {
        // Display slot name with shortcut
        mvwprintw(equipmentWindow, y, 2, "%c - %-15s: ", slotInfo.shortcut, slotInfo.name);

        // Get equipped item in this slot
        Item* equippedItem = player.get_equipped_item(slotInfo.slot);
        if (equippedItem)
        {
            // Display equipped item
            wattron(equipmentWindow, COLOR_PAIR(equippedItem->actorData.color));
            wprintw(equipmentWindow, "%s", equippedItem->actorData.name.c_str());
            wattroff(equipmentWindow, COLOR_PAIR(equippedItem->actorData.color));

            // Show value if applicable
            if (equippedItem->value > 0)
            {
                wattron(equipmentWindow, COLOR_PAIR(YELLOW_BLACK_PAIR));
                wprintw(equipmentWindow, " (%d gp)", equippedItem->value);
                wattroff(equipmentWindow, COLOR_PAIR(YELLOW_BLACK_PAIR));
            }
        }
        else
        {
            // Empty slot
            mvwprintw(equipmentWindow, y, 20, "- empty -");
        }
        y++;
    }
}

void InventoryUI::display_instructions(GameContext& ctx)
{
    // Calculate total weight (simplified - assume 1 weight per item for now)
    int totalWeight = 0;
    totalWeight = static_cast<int>(get_item_count(ctx.player->inventory_data));
    
    mvwprintw(
        inventoryWindow,
        LINES - INSTRUCTIONS_Y_OFFSET,
        2,
        "Weight: %d stones | Backpack: (a-z), Equipment: (H/N/B/etc), ESC=exit",
        totalWeight
    );
    
    mvwprintw(equipmentWindow, LINES - 3, 2, "Equipment Slots");
    mvwprintw(equipmentWindow, LINES - 2, 2, "Select by letter");
}

bool InventoryUI::handle_inventory_input(Player& player, GameContext& ctx)
{
    int input = getch();
    
    if (input == static_cast<int>(Controls::ESCAPE))
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, "Inventory closed.", true);
        return false; // Exit inventory
    }
    
    // Handle backpack item selection (a-z)
    if (input >= 'a' && input <= 'z')
    {
        return handle_backpack_selection(player, input - 'a', ctx);
    }
    
    // Handle equipment slot selection
    return handle_equipment_selection(player, input, ctx);
}

bool InventoryUI::handle_backpack_selection(Player& player, int itemIndex, GameContext& ctx)
{
    if (is_inventory_empty(player.inventory_data))
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, "Your backpack is empty.", true);
        return true;
    }
    
    // Build list of valid items (skipping null entries)
    std::vector<Item*> validItems;
    for (const auto& item : player.inventory_data.items)
    {
        if (item)
        {
            validItems.push_back(item.get());
        }
    }
    
    // Debug logging
    ctx.message_system->log("Inventory selection debug:");
    ctx.message_system->log("Selected index: " + std::to_string(itemIndex));
    ctx.message_system->log("Valid items count: " + std::to_string(validItems.size()));
    ctx.message_system->log("Total inventory slots: " + std::to_string(player.inventory_data.items.size()));
    
    if (itemIndex >= 0 && itemIndex < static_cast<int>(validItems.size()))
    {
        Item* selectedItem = validItems[itemIndex];
        
        ctx.message_system->log("Selected item: " + selectedItem->actorData.name);
        ctx.message_system->log("Has pickable: " + std::string(selectedItem->pickable ? "YES" : "NO"));
        
        if (selectedItem && selectedItem->pickable)
        {
            Item* itemPtr = selectedItem;

            ctx.message_system->log("Attempting to use item...");
            bool itemUsed = selectedItem->pickable->use(*selectedItem, player, ctx);
            ctx.message_system->log("Item use result: " + std::string(itemUsed ? "SUCCESS" : "FAILED"));
            
            if (itemUsed)
            {
                *ctx.game_status = GameStatus::NEW_TURN;
                
                // Check if the item still exists in inventory (not consumed)
                bool itemStillExists = false;
                for (const auto& item : player.inventory_data.items)
                {
                    if (item.get() == itemPtr)
                    {
                        itemStillExists = true;
                        break;
                    }
                }
                
                if (!itemStillExists)
                {
                    return false; // Exit inventory
                }
            }
            
            return true;
        }
        else
        {
            ctx.message_system->log("Item has no pickable component!");
        }
    }
    else
    {
        ctx.message_system->log("Index out of bounds!");
    }
    
    ctx.message_system->message(WHITE_BLACK_PAIR, "Invalid backpack selection.", true);
    return true;
}

bool InventoryUI::handle_equipment_selection(Player& player, int input, GameContext& ctx)
{
    // Map input characters to equipment slots
    EquipmentSlot targetSlot = EquipmentSlot::NONE;
    
    switch (input)
    {
        case 'H': targetSlot = EquipmentSlot::HEAD; break;
        case 'N': targetSlot = EquipmentSlot::NECK; break;
        case 'B': targetSlot = EquipmentSlot::BODY; break;
        case 'G': targetSlot = EquipmentSlot::GIRDLE; break;
        case 'C': targetSlot = EquipmentSlot::CLOAK; break;
        case 'R': targetSlot = EquipmentSlot::RIGHT_HAND; break;
        case 'L': targetSlot = EquipmentSlot::LEFT_HAND; break;
        case ')': targetSlot = EquipmentSlot::RIGHT_RING; break;
        case '(': targetSlot = EquipmentSlot::LEFT_RING; break;
        case '[': targetSlot = EquipmentSlot::BRACERS; break;
        case ']': targetSlot = EquipmentSlot::GAUNTLETS; break;
        case 'F': targetSlot = EquipmentSlot::BOOTS; break;
        case 'M': targetSlot = EquipmentSlot::MISSILE_WEAPON; break;
        case 'A': targetSlot = EquipmentSlot::MISSILES; break;
        case 'T': targetSlot = EquipmentSlot::TOOL; break;
        default:
            ctx.message_system->message(WHITE_BLACK_PAIR, "Invalid equipment selection.", true);
            return true; // Stay in inventory
    }
    
    // Get currently equipped item in this slot
    Item* equippedItem = player.get_equipped_item(targetSlot);
    
    if (equippedItem)
    {
        // Unequip the item - now no cast needed since player is mutable
        player.unequip_item(targetSlot, ctx);
        ctx.message_system->message(WHITE_BLACK_PAIR, "You unequipped the " + equippedItem->get_name() + ".", true);
    }
    else
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, "That equipment slot is empty.", true);
    }
    
    return true; // Stay in inventory
}

void InventoryUI::restore_game_display(GameContext& ctx)
{
    clear();
    refresh();
    ctx.rendering_manager->restore_game_display();
}
