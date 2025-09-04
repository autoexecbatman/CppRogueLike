#include "InventoryUI.h"
#include "../Actor/Actor.h"
#include "../ActorTypes/Player.h"
#include "../Actor/Pickable.h"
#include "../Game.h"
#include "../Controls/Controls.h"
#include "../Colors/Colors.h"
#include <algorithm>

InventoryUI::InventoryUI() : inventoryWindow(nullptr), equipmentWindow(nullptr)
{
}

InventoryUI::~InventoryUI()
{
    destroy_windows();
}

void InventoryUI::display(Creature& owner)
{
    auto* player = dynamic_cast<Player*>(&owner);
    
    while (true)
    {
        create_windows();
        
        // Display content - separated backpack and equipment
        display_inventory_items(player);
        display_equipment_slots(player);
        display_instructions();
        
        refresh_windows();
        
        // Handle input - returns false to exit inventory
        if (!handle_inventory_input(owner, player))
        {
            break;
        }
        
        destroy_windows();
    }
    
    restore_game_display();
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

std::vector<std::pair<Item*, bool>> InventoryUI::build_item_list(Creature& owner, Player* player)
{
    std::vector<std::pair<Item*, bool>> allItems;
    
    // Add equipped items first (with [E] markers)
    if (player)
    {
        for (const auto& equipped : player->equippedItems)
        {
            if (equipped.item)
            {
                allItems.emplace_back(equipped.item.get(), true);
            }
        }
    }
    
    // Add regular inventory items
    for (const auto& item : game.container->get_inventory_mutable())
    {
        if (item)
        {
            allItems.emplace_back(item.get(), false);
        }
    }
    
    return allItems;
}

void InventoryUI::display_inventory_items(Player* player)
{
    // Update title with backpack item count
    if (player)
    {
        size_t backpackItems = player->container->get_item_count();
        size_t maxItems = player->container->get_capacity();
        mvwprintw(inventoryWindow, 0, 2, " BACKPACK (%zu/%zu) ", backpackItems, maxItems);
    }
    
    int y = 2;
    char shortcut = 'a';
    
    // Display only backpack items (not equipped items)
    const auto& backpackItems = player->container->get_inventory();
    
    if (backpackItems.empty())
    {
        mvwprintw(inventoryWindow, y, 2, "Your backpack is empty.");
        return;
    }
    
    for (size_t i = 0; i < backpackItems.size() && shortcut <= 'z'; i++)
    {
        Item* item = backpackItems[i].get();
        
        if (item)
        {
            mvwprintw(inventoryWindow, y, 2, "%c) ", shortcut);
            
            show_item_info(item, y);
            
            y++;
            shortcut++;
        }
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
    
    // Show weapon damage if applicable
    if (auto* weapon = dynamic_cast<Weapon*>(item->pickable.get()))
    {
        std::string damageInfo = " [" + weapon->roll + (weapon->is_ranged() ? " rng dmg]" : " dmg]");
        wattron(inventoryWindow, COLOR_PAIR(WHITE_BLACK_PAIR));
        wprintw(inventoryWindow, "%s", damageInfo.c_str());
        wattroff(inventoryWindow, COLOR_PAIR(WHITE_BLACK_PAIR));
    }
}

void InventoryUI::display_equipment_slots(Player* player)
{
if (!player) return;

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
Item* equippedItem = player->get_equipped_item(slotInfo.slot);
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

void InventoryUI::display_instructions()
{
    // Calculate total weight (simplified - assume 1 weight per item for now)
    int totalWeight = 0;
    if (game.player && game.player->container)
    {
        totalWeight = static_cast<int>(game.player->container->get_item_count());
    }
    
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

bool InventoryUI::handle_inventory_input(Creature& owner, Player* player)
{
    int input = getch();
    
    if (input == static_cast<int>(Controls::ESCAPE))
    {
        game.message(WHITE_BLACK_PAIR, "Inventory closed.", true);
        return false; // Exit inventory
    }
    
    // Handle backpack item selection (a-z)
    if (input >= 'a' && input <= 'z')
    {
        return handle_backpack_selection(owner, player, input - 'a');
    }
    
    // Handle equipment slot selection
    return handle_equipment_selection(owner, player, input);
}

bool InventoryUI::handle_backpack_selection(Creature& owner, Player* player, int itemIndex)
{
    const auto& backpackItems = game.container->get_inventory_mutable();
    
    if (itemIndex >= 0 && itemIndex < static_cast<int>(backpackItems.size()))
    {
        Item* selectedItem = backpackItems[itemIndex].get();
        
        if (selectedItem && selectedItem->pickable)
        {
            // Store item pointer to detect if it was consumed
            Item* itemPtr = selectedItem;
            
            // Try to use item
            bool itemUsed = selectedItem->pickable->use(*selectedItem, owner);
            
            if (itemUsed)
            {
                game.gameStatus = Game::GameStatus::NEW_TURN;
                
                // Check if the item still exists in inventory (not consumed)
                bool itemStillExists = false;
                for (const auto& item : game.container->get_inventory_mutable())
                {
                    if (item.get() == itemPtr)
                    {
                        itemStillExists = true;
                        break;
                    }
                }
                
                // If item was consumed, exit inventory
                if (!itemStillExists)
                {
                    return false; // Exit inventory
                }
            }
            
            return true; // Stay in inventory
        }
    }
    
    game.message(WHITE_BLACK_PAIR, "Invalid backpack selection.", true);
    return true; // Stay in inventory
}

bool InventoryUI::handle_equipment_selection(Creature& owner, Player* player, int input)
{
    if (!player) return true;
    
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
            game.message(WHITE_BLACK_PAIR, "Invalid equipment selection.", true);
            return true; // Stay in inventory
    }
    
    // Get currently equipped item in this slot
    Item* equippedItem = player->get_equipped_item(targetSlot);
    
    if (equippedItem)
    {
        // Unequip the item
        player->unequip_item(targetSlot);
        game.message(WHITE_BLACK_PAIR, "You unequipped the " + equippedItem->actorData.name + ".", true);
    }
    else
    {
        game.message(WHITE_BLACK_PAIR, "That equipment slot is empty.", true);
    }
    
    return true; // Stay in inventory
}

void InventoryUI::restore_game_display()
{
    clear();
    refresh();
    game.restore_game_display();
}
