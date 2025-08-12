#pragma once
#include <curses.h>
#include <vector>
#include <utility>

class Creature;
class Item;
class Player;

class InventoryUI
{
public:
    InventoryUI();
    ~InventoryUI();
    
    // Main entry point - displays inventory and handles all interaction
    void display(Creature& owner);
    
private:
    // Window management
    void createWindows();
    void destroyWindows();
    void refreshWindows();
    
    // Display methods
    void displayInventoryItems(Player* player, const std::vector<std::pair<Item*, bool>>& allItems);
    void displayEquipmentSlots(Player* player);
    void displayInstructions(int itemCount);
    
    // Input handling
    bool handleInput(Creature& owner, Player* player, const std::vector<std::pair<Item*, bool>>& allItems, int itemCount);
    bool handleItemSelection(Creature& owner, Player* player, const std::vector<std::pair<Item*, bool>>& allItems, int itemIndex);
    bool handleToggleGrip(Player* player);
    
    // Helper methods
    std::vector<std::pair<Item*, bool>> buildItemList(Creature& owner, Player* player);
    void showItemInfo(Item* item, int y);
    void restoreGameDisplay();
    
    // Window pointers
    WINDOW* inventoryWindow;
    WINDOW* equipmentWindow;
    
    // Layout calculation methods
    int getInventoryWidth() const { return COLS / 2; }
    int getEquipmentWidth() const { return COLS - getInventoryWidth(); }
    
    // Layout constants
    static constexpr int INSTRUCTIONS_Y_OFFSET = 2;
};
