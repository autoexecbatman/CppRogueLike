#pragma once
#include <curses.h>
#include <vector>
#include <utility>

class Creature;
class Item;
class Player;
struct GameContext;

class InventoryUI
{
public:
    InventoryUI();
    ~InventoryUI();
    
    // Main entry point - displays inventory and handles all interaction
    void display(Player& player, GameContext& ctx);
    
private:
    // Window management
    void create_windows();
    void destroy_windows();
    void refresh_windows();
    
    // Display methods
    void display_inventory_items(const Player& player);
    void display_equipment_slots(const Player& player);
    void display_instructions(GameContext& ctx);
    
    // Input handling
    bool handle_inventory_input(Player& player, GameContext& ctx);
    bool handle_backpack_selection(Player& player, int itemIndex, GameContext& ctx);
    bool handle_equipment_selection(Player& player, int input, GameContext& ctx);
    
    // Helper methods
    std::vector<std::pair<Item*, bool>> build_item_list(const Player& player);
    void show_item_info(Item* item, int y);
    void restore_game_display(GameContext& ctx);
    
    // Window pointers
    WINDOW* inventoryWindow;
    WINDOW* equipmentWindow;
    
    // Layout calculation methods
    int get_inventory_width() const { return COLS / 2; }
    int get_equipment_width() const { return COLS - get_inventory_width(); }
    
    // Layout constants
    static constexpr int INSTRUCTIONS_Y_OFFSET = 2;
};
