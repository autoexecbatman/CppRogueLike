#include <span>
#include <ranges>
#include <memory>

#include "MenuBuy.h"
#include "../Game.h"
#include "../ActorTypes/Player.h"
#include "../Actor/InventoryOperations.h"

using namespace InventoryOperations; // For clean function calls

void MenuBuy::populate_items(std::span<std::unique_ptr<Item>> inventory)
{
	menuItems.clear();
	
	// Handle empty inventory case
	if (inventory.empty())
	{
		menuItems.push_back("No items for sale");
		return;
	}
	
	for (const auto& item : inventory)
	{
		if (item)
		{
			// Display item name with right-aligned gold value
			std::string itemName = item->actorData.name;
			std::string goldText = "(" + std::to_string(item->value) + "g)";
			
			// Pad to align gold values (assuming max name length ~20)
			size_t totalWidth = 28;
			size_t padding = totalWidth > (itemName.length() + goldText.length()) ? 
							 totalWidth - itemName.length() - goldText.length() : 1;
			
			std::string itemDisplay = itemName + std::string(padding, ' ') + goldText;
			menuItems.push_back(itemDisplay);
		}
		else
		{
			game.log("MenuBuy Item is null.");
			std::exit(EXIT_FAILURE);
		}
	}
}

MenuBuy::MenuBuy(Creature& buyer) : buyer{ buyer }
{
	// Use full screen dimensions
	menu_height = static_cast<size_t>(LINES);
	menu_width = static_cast<size_t>(COLS);
	
	populate_items(buyer.inventory_data.items);
	menu_new(menu_height, menu_width, menu_starty, menu_startx);
}

MenuBuy::~MenuBuy()
{
	menu_delete();
}

void MenuBuy::menu_print_state(size_t state)
{
	if (state >= menuItems.size()) return; // Bounds check
	
	if (currentState == state)
	{
		menu_highlight_on();
	}
	menu_print(2, state + 4, menu_get_string(state)); // Start at row 4, indent from left border
	if (currentState == state)
	{
		menu_highlight_off();
	}
}

void MenuBuy::draw_content()
{
	// Only redraw if content changed
	populate_items(buyer.inventory_data.items);
	
	// Draw all menu items efficiently
	for (size_t i{ 0 }; i < menuItems.size(); ++i)
	{
		menu_print_state(i);
	}
}

void MenuBuy::draw()
{
	menu_clear();
	box(menuWindow, 0, 0);
	
	// Title centered on screen
	size_t title_x = (menu_width - 9) / 2; // Center "Buy Items" (9 chars)
	mvwprintw(menuWindow, 1, title_x, "Buy Items");
	
	// Instructions
	mvwprintw(menuWindow, 2, 2, "Use UP/DOWN or W/S to navigate, ENTER to buy, ESC to exit");
	
	populate_items(buyer.inventory_data.items);
	
	// Start items at row 4 to leave space for title and instructions
	for (size_t i{ 0 }; i < menuItems.size(); ++i)
	{
		menu_print_state(i);
	}
	menu_refresh();
}

void MenuBuy::on_key(int key)
{
	switch (key)
	{
		case KEY_UP:
		case 'w':
			// Don't allow navigation if no items for sale
			if (buyer.inventory_data.items.empty()) return;
			if (menuItems.empty())
			{
				return;
			}
			currentState = (currentState + menuItems.size() - 1) % menuItems.size();
			menu_mark_dirty(); // Mark for redraw
			break;
		case KEY_DOWN:
		case 's':
			// Don't allow navigation if no items for sale
			if (buyer.inventory_data.items.empty()) return;
			if (menuItems.empty())
			{
				return;
			}
			currentState = (currentState + 1) % menuItems.size();
			menu_mark_dirty(); // Mark for redraw
			break;
		case 27: // ESC
			menu_set_run_false();
			break;
		case 10: // ENTER
			// Only allow buying if shopkeeper actually has items
			if (!buyer.inventory_data.items.empty() && !menuItems.empty())
			{
				handle_buy(menuWindow, buyer, *game.player);
				menu_mark_dirty(); // Redraw after purchase
			}
			else
			{
				game.message(WHITE_BLACK_PAIR, "No items for sale.", true);
			}
			break;
	}
}

void MenuBuy::menu()
{
	// Initial draw
	draw();
	
	while (run)
	{
		// Only redraw if needed (navigation/state change)
		if (menu_needs_redraw())
		{
			draw();
		}
		
		menu_key_listen();
		on_key(keyPress);
	}
	
	// Clear screen when exiting
	clear();
	refresh();
}

void MenuBuy::handle_buy(WINDOW* tradeWin, Creature& shopkeeper, Player& seller)
{
    if (is_inventory_empty(shopkeeper.inventory_data) || 
        currentState >= get_item_count(shopkeeper.inventory_data))
    {
        game.message(WHITE_BLACK_PAIR, "Invalid selection.", true);
        return;
    }
    
    const Item* item = get_item_at(shopkeeper.inventory_data, currentState);
    if (!item)
    {
        game.message(WHITE_BLACK_PAIR, "Invalid selection.", true);
        return;
    }
    
    if (seller.get_gold() >= item->value)
    {
        // Remove item from shopkeeper and add to seller
        auto removed_item = remove_item_at(shopkeeper.inventory_data, currentState);
        if (removed_item.has_value())
        {
            seller.adjust_gold(-item->value);
            shopkeeper.adjust_gold(item->value);
            
            add_item(seller.inventory_data, std::move(*removed_item));
            
            // Adjust currentState bounds
            if (currentState >= get_item_count(shopkeeper.inventory_data) && !is_inventory_empty(shopkeeper.inventory_data))
            {
                currentState = get_item_count(shopkeeper.inventory_data) - 1;
            }
            
            game.message(WHITE_BLACK_PAIR, "Item purchased successfully.", true);
        }
        else
        {
            game.message(WHITE_BLACK_PAIR, "Transaction failed.", true);
        }
    }
    else
    {
        game.message(WHITE_BLACK_PAIR, "Insufficient currency.", true);
    }
}