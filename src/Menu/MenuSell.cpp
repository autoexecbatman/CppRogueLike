#include "MenuSell.h"
#include "../Game.h"
#include "../Actor/Actor.h"
#include "BaseMenu.h"

void MenuSell::populate_items(std::span<std::unique_ptr<Item>> item)
{
	menuItems.clear();
	
	// Handle empty inventory case
	if (item.empty())
	{
		menuItems.push_back("No items to sell");
		return;
	}
	
	for (const auto& item : item)
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
			game.log("MenuSell Item is null.");
			std::exit(EXIT_FAILURE);
		}
	}
}

void MenuSell::menu_print_state(size_t state)
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

void MenuSell::handle_sell(WINDOW* tradeWin, Creature& shopkeeper, Creature& seller)
{
    if (seller.container->is_empty() || currentState >= seller.container->get_item_count())
    {
        game.message(WHITE_BLACK_PAIR, "Invalid selection.", true);
        return;
    }
    
    const Item* item = seller.container->get_item_at(currentState);
    if (!item)
    {
        game.log("Error: Attempted to sell a null item.");
        game.message(WHITE_BLACK_PAIR, "Error: Invalid item.", true);
        return;
    }
    
    auto price = item->value;
    
    if (shopkeeper.get_gold() >= price)
    {
        // Remove item from seller
        auto removed_item = seller.container->remove_at(currentState);
        if (removed_item.has_value())
        {
            shopkeeper.adjust_gold(-price);
            seller.adjust_gold(price);
            
            // Add item to shopkeeper
            auto add_result = shopkeeper.container->add(std::move(*removed_item));
            if (!add_result.has_value())
            {
                // Handle failure case - return item to seller
                seller.container->add(std::move(*removed_item));
                shopkeeper.adjust_gold(price);
                seller.adjust_gold(-price);
                game.message(WHITE_BLACK_PAIR, "Shopkeeper's inventory is full.", true);
                return;
            }
            
            // Adjust currentState bounds
            if (currentState >= seller.container->get_item_count() && !seller.container->is_empty())
            {
                currentState = seller.container->get_item_count() - 1;
            }
            
            game.message(WHITE_BLACK_PAIR, "Item sold successfully.", true);
        }
        else
        {
            game.message(WHITE_BLACK_PAIR, "Transaction failed.", true);
        }
    }
    else
    {
        game.log("Shopkeeper does not have enough gold to buy the item.");
        game.message(WHITE_BLACK_PAIR, "Shopkeeper does not have enough gold to buy the item.", true);
    }
}

MenuSell::MenuSell(Creature& shopkeeper, Creature& player) : player(player), shopkeeper(shopkeeper)
{
	// Use full screen dimensions
	menu_height = static_cast<size_t>(LINES);
	menu_width = static_cast<size_t>(COLS);
	
	populate_items(player.container->get_inventory_mutable());
	menu_new(menu_height, menu_width, menu_starty, menu_startx);
	
	// Reset currentState if inventory is empty
	if (player.container->get_inventory_mutable().empty())
	{
		currentState = 0;
	}
	else if (currentState >= player.container->get_inventory_mutable().size())
	{
		currentState = player.container->get_inventory_mutable().empty() ? 0 : player.container->get_inventory_mutable().size() - 1;
	}
}

MenuSell::~MenuSell()
{
	menu_delete();
}

void MenuSell::draw_content()
{
	// Only redraw if content changed
	populate_items(player.container->get_inventory_mutable());
	
	// Validate currentState after repopulating
	if (player.container->get_inventory_mutable().empty())
	{
		currentState = 0;
	}
	else if (currentState >= player.container->get_inventory_mutable().size())
	{
		currentState = player.container->get_inventory_mutable().size() - 1;
	}
	
	// Draw all menu items efficiently
	for (size_t i{ 0 }; i < menuItems.size(); ++i)
	{
		menu_print_state(i);
	}
}

void MenuSell::draw()
{
	menu_clear();
	box(menuWindow, 0, 0);
	
	// Title centered on screen
	size_t title_x = (menu_width - 10) / 2; // Center "Sell Items" (10 chars)
	mvwprintw(menuWindow, 1, title_x, "Sell Items");
	
	// Instructions
	mvwprintw(menuWindow, 2, 2, "Use UP/DOWN or W/S to navigate, ENTER to sell, ESC to exit");
	
	populate_items(player.container->get_inventory_mutable());
	
	// Validate currentState after repopulating
	if (player.container->get_inventory_mutable().empty())
	{
		currentState = 0;
	}
	else if (currentState >= player.container->get_inventory_mutable().size())
	{
		currentState = player.container->get_inventory_mutable().size() - 1;
	}
	
	// Start items at row 4 to leave space for title and instructions
	for (size_t i{ 0 }; i < menuItems.size(); ++i)
	{
		menu_print_state(i);
	}
	menu_refresh();
}

void MenuSell::on_key(int key)
{
	switch (key)
	{
	case KEY_UP:
	case 'w':
		// Don't allow navigation if no sellable items
		if (player.container->get_inventory_mutable().empty()) return;
		if (menuItems.empty()) return; // Check for empty menu
		currentState = (currentState + menuItems.size() - 1) % menuItems.size();
		menu_mark_dirty(); // Mark for redraw
		break;
	case KEY_DOWN:
	case 's':
		// Don't allow navigation if no sellable items
		if (player.container->get_inventory_mutable().empty()) return;
		if (menuItems.empty()) return; // Check for empty menu
		currentState = (currentState + 1) % menuItems.size();
		menu_mark_dirty(); // Mark for redraw
		break;
	case 10: // Enter key
	{
		// Only allow selling if player actually has items
		if (!player.container->get_inventory_mutable().empty() && !menuItems.empty())
		{
			handle_sell(menuWindow, shopkeeper, player);
			menu_mark_dirty(); // Redraw after sale
		}
		else
		{
			game.message(WHITE_BLACK_PAIR, "No items to sell.", true);
		}
	}
	break;
	case 27: // Escape key
	{
		menu_set_run_false();
	}
	break;
	}
}

void MenuSell::menu()
{
	// Initial draw
	draw();
	
	while(run)
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
