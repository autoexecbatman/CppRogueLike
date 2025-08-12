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
    destroyWindows();
}

void InventoryUI::display(Creature& owner)
{
    auto* player = dynamic_cast<Player*>(&owner);
    
    while (true)
    {
        createWindows();
        
        // Build combined item list
        auto allItems = buildItemList(owner, player);
        
        // Display content
        displayInventoryItems(player, allItems);
        displayEquipmentSlots(player);
        displayInstructions(static_cast<int>(allItems.size()));
        
        refreshWindows();
        
        // Handle input - returns false to exit inventory
        if (!handleInput(owner, player, allItems, static_cast<int>(allItems.size())))
        {
            break;
        }
        
        destroyWindows();
    }
    
    restoreGameDisplay();
}

void InventoryUI::createWindows()
{
    clear();
    refresh();
    
    inventoryWindow = newwin(LINES, getInventoryWidth(), 0, 0);
    equipmentWindow = newwin(LINES, getEquipmentWidth(), 0, getInventoryWidth());
    
    box(inventoryWindow, 0, 0);
    box(equipmentWindow, 0, 0);
    
    // Window titles
    wattron(inventoryWindow, A_BOLD);
    mvwprintw(inventoryWindow, 0, 2, " INVENTORY ");
    wattroff(inventoryWindow, A_BOLD);
    
    wattron(equipmentWindow, A_BOLD);
    mvwprintw(equipmentWindow, 0, 2, " EQUIPMENT ");
    wattroff(equipmentWindow, A_BOLD);
}

void InventoryUI::destroyWindows()
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

void InventoryUI::refreshWindows()
{
    wrefresh(inventoryWindow);
    wrefresh(equipmentWindow);
}

std::vector<std::pair<Item*, bool>> InventoryUI::buildItemList(Creature& owner, Player* player)
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
    for (const auto& item : owner.container->inv)
    {
        if (item)
        {
            allItems.emplace_back(item.get(), false);
        }
    }
    
    return allItems;
}

void InventoryUI::displayInventoryItems(Player* player, const std::vector<std::pair<Item*, bool>>& allItems)
{
    // Update title with item count
    if (player)
    {
        size_t totalItems = player->container->inv.size() + player->equippedItems.size();
        size_t maxItems = player->container->invSize;
        mvwprintw(inventoryWindow, 0, 2, " INVENTORY (%zu/%zu) ", totalItems, maxItems);
    }
    
    int y = 2;
    char shortcut = 'a';
    
    if (allItems.empty())
    {
        mvwprintw(inventoryWindow, y, 2, "You are not carrying anything.");
        return;
    }
    
    for (size_t i = 0; i < allItems.size() && shortcut <= 'z'; i++)
    {
        Item* item = allItems[i].first;
        bool isEquipped = allItems[i].second;
        
        if (item)
        {
            mvwprintw(inventoryWindow, y, 2, "%c) ", shortcut);
            
            // Show equipped marker
            if (isEquipped)
            {
                wattron(inventoryWindow, COLOR_PAIR(WHITE_GREEN_PAIR));
                wprintw(inventoryWindow, "[E] ");
                wattroff(inventoryWindow, COLOR_PAIR(WHITE_GREEN_PAIR));
            }
            
            showItemInfo(item, y);
            
            y++;
            shortcut++;
        }
    }
}

void InventoryUI::showItemInfo(Item* item, int y)
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
        std::string damageInfo = " [" + weapon->roll + (weapon->isRanged() ? " rng dmg]" : " dmg]");
        wattron(inventoryWindow, COLOR_PAIR(WHITE_BLACK_PAIR));
        wprintw(inventoryWindow, "%s", damageInfo.c_str());
        wattroff(inventoryWindow, COLOR_PAIR(WHITE_BLACK_PAIR));
    }
}

void InventoryUI::displayEquipmentSlots(Player* player)
{
    if (!player) return;
    
    int y = 2;
    
    // Main Hand
    mvwprintw(equipmentWindow, y++, 2, "Main Hand:");
    Item* mainHand = player->getEquippedItem(EquipmentSlot::MAIN_HAND);
    if (mainHand)
    {
        mvwprintw(equipmentWindow, y, 4, "[W] ");
        wattron(equipmentWindow, COLOR_PAIR(mainHand->actorData.color));
        wprintw(equipmentWindow, "%s", mainHand->actorData.name.c_str());
        wattroff(equipmentWindow, COLOR_PAIR(mainHand->actorData.color));
        
        // Check if two-handed
        for (const auto& equipped : player->equippedItems)
        {
            if (equipped.slot == EquipmentSlot::MAIN_HAND && equipped.twoHandedGrip)
            {
                wprintw(equipmentWindow, " (2H)");
                break;
            }
        }
    }
    else
    {
        mvwprintw(equipmentWindow, y, 4, "- empty -");
    }
    y += 2;
    
    // Off Hand
    mvwprintw(equipmentWindow, y++, 2, "Off Hand:");
    Item* offHand = player->getEquippedItem(EquipmentSlot::OFF_HAND);
    if (offHand)
    {
        mvwprintw(equipmentWindow, y, 4, "[S] ");
        wattron(equipmentWindow, COLOR_PAIR(offHand->actorData.color));
        wprintw(equipmentWindow, "%s", offHand->actorData.name.c_str());
        wattroff(equipmentWindow, COLOR_PAIR(offHand->actorData.color));
    }
    else
    {
        mvwprintw(equipmentWindow, y, 4, "- empty -");
    }
    y += 2;
    
    // Armor
    mvwprintw(equipmentWindow, y++, 2, "Armor:");
    Item* armor = player->getEquippedItem(EquipmentSlot::ARMOR);
    if (armor)
    {
        mvwprintw(equipmentWindow, y, 4, "[A] ");
        wattron(equipmentWindow, COLOR_PAIR(armor->actorData.color));
        wprintw(equipmentWindow, "%s", armor->actorData.name.c_str());
        wattroff(equipmentWindow, COLOR_PAIR(armor->actorData.color));
    }
    else
    {
        mvwprintw(equipmentWindow, y, 4, "- empty -");
    }
}

void InventoryUI::displayInstructions(int itemCount)
{
    mvwprintw(
        inventoryWindow,
        LINES - INSTRUCTIONS_Y_OFFSET,
        2,
        "Select item (a-%c), T=toggle grip, ESC=exit",
        itemCount > 0 ? static_cast<char>('a' + itemCount - 1) : 'a'
    );
    
    mvwprintw(equipmentWindow, LINES - 3, 2, "Equipment Slots");
    mvwprintw(equipmentWindow, LINES - 2, 2, "Auto-updated");
}

bool InventoryUI::handleInput(Creature& owner, Player* player, const std::vector<std::pair<Item*, bool>>& allItems, int itemCount)
{
    int input = getch();
    
    if (input == static_cast<int>(Controls::ESCAPE))
    {
        game.message(WHITE_BLACK_PAIR, "Inventory closed.", true);
        return false; // Exit inventory
    }
    else if (input == static_cast<int>(Controls::TOGGLE_GRIP))
    {
        return handleToggleGrip(player); // Returns true to stay in inventory
    }
    else if (input >= 'a' && input <= 'z' && input < 'a' + itemCount)
    {
        int itemIndex = input - 'a';
        return handleItemSelection(owner, player, allItems, itemIndex);
    }
    else
    {
        game.message(WHITE_BLACK_PAIR, "Invalid selection.", true);
        return true; // Stay in inventory
    }
}

bool InventoryUI::handleItemSelection(Creature& owner, Player* player, const std::vector<std::pair<Item*, bool>>& allItems, int itemIndex)
{
    if (itemIndex >= 0 && itemIndex < static_cast<int>(allItems.size()))
    {
        Item* selectedItem = allItems[itemIndex].first;
        bool isEquipped = allItems[itemIndex].second;
        
        if (selectedItem && selectedItem->pickable)
        {
            if (isEquipped)
            {
                // Unequip item
                if (player)
                {
                    for (const auto& equipped : player->equippedItems)
                    {
                        if (equipped.item.get() == selectedItem)
                        {
                            std::string itemName = selectedItem->actorData.name;
                            player->unequipItem(equipped.slot);
                            game.message(WHITE_BLACK_PAIR, "You unequipped the " + itemName + ".", true);
                            break;
                        }
                    }
                }
            }
            else
            {
                // Equip/use item
                selectedItem->pickable->use(*selectedItem, owner);
            }
            
            game.gameStatus = Game::GameStatus::NEW_TURN;
        }
    }
    
    return true; // Stay in inventory to see changes
}

bool InventoryUI::handleToggleGrip(Player* player)
{
    if (!player)
    {
        game.message(WHITE_BLACK_PAIR, "Cannot toggle grip.", true);
        return true;
    }
    
    Item* mainHandWeapon = player->getEquippedItem(EquipmentSlot::MAIN_HAND);
    if (!mainHandWeapon)
    {
        game.message(WHITE_BLACK_PAIR, "You have no weapon equipped to toggle grip.", true);
        return true;
    }
    
    // Find the equipped item entry
    auto it = std::find_if(player->equippedItems.begin(), player->equippedItems.end(),
        [](const EquippedItem& equipped) { return equipped.slot == EquipmentSlot::MAIN_HAND; });
    
    if (it != player->equippedItems.end())
    {
        auto* weapon = dynamic_cast<Weapon*>(mainHandWeapon->pickable.get());
        if (weapon && weapon->isVersatile())
        {
            // Toggle the two-handed grip
            it->twoHandedGrip = !it->twoHandedGrip;
            
            if (it->twoHandedGrip)
            {
                player->unequipItem(EquipmentSlot::OFF_HAND);
                game.message(WHITE_BLACK_PAIR, "You grip the " + mainHandWeapon->actorData.name + " with both hands.", true);
            }
            else
            {
                game.message(WHITE_BLACK_PAIR, "You grip the " + mainHandWeapon->actorData.name + " with one hand.", true);
            }
            
            // Update weapon damage roll
            std::string newRoll = player->getEquippedWeaponDamageRoll();
            player->attacker->roll = newRoll;
        }
        else
        {
            game.message(WHITE_BLACK_PAIR, "This weapon cannot be used two-handed.", true);
        }
    }
    
    return true; // Stay in inventory
}

void InventoryUI::restoreGameDisplay()
{
    clear();
    refresh();
    game.restore_game_display();
}
