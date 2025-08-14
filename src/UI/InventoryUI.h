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
    void create_windows();
    void destroy_windows();
    void refresh_windows();
    
    // Display methods
    void display_inventory_items(Player* player);
    void display_equipment_slots(Player* player);
    void display_instructions();
    
    // Input handling
    bool handle_inventory_input(Creature& owner, Player* player);
    bool handle_backpack_selection(Creature& owner, Player* player, int itemIndex);
    bool handle_equipment_selection(Creature& owner, Player* player, int input);
    
    // Helper methods
    std::vector<std::pair<Item*, bool>> build_item_list(Creature& owner, Player* player);
    void show_item_info(Item* item, int y);
    void restore_game_display();
    
    // Window pointers
    WINDOW* inventoryWindow;
    WINDOW* equipmentWindow;
    
    // Layout calculation methods
    int get_inventory_width() const { return COLS / 2; }
    int get_equipment_width() const { return COLS - get_inventory_width(); }
    
    // Layout constants
    static constexpr int INSTRUCTIONS_Y_OFFSET = 2;
};
