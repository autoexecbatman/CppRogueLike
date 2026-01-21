#include "MenuBuy.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"
#include "../ActorTypes/Player.h"
#include "../Actor/InventoryOperations.h"
#include "../Systems/ShopKeeper.h"

using namespace InventoryOperations;

void MenuBuy::populate_items()
{
	menuItems.clear();
	
	// Handle empty inventory case
	if (is_inventory_empty(shopkeeper.shop_inventory))
	{
		menuItems.push_back("No items for sale");
		return;
	}

	for (const auto& item : shopkeeper.shop_inventory.items)
	{
		if (item)
		{
			// Display item name with buy price
			std::string itemName = item->actorData.name;
			int price = shopkeeper.get_buy_price(*item);
			std::string goldText = "(" + std::to_string(price) + "g)";

			// Pad to align gold values
			size_t totalWidth = 28;
			size_t padding = totalWidth > (itemName.length() + goldText.length()) ? totalWidth - itemName.length() - goldText.length() : 1;

			std::string itemDisplay = itemName + std::string(padding, ' ') + goldText;
			menuItems.push_back(itemDisplay);
		}
	}
}

MenuBuy::MenuBuy(GameContext& ctx, Creature& buyer, ShopKeeper& shopkeeper)
	: buyer{ buyer }, shopkeeper{ shopkeeper }, ctx{ ctx }
{
	// Use full screen dimensions
	menu_height = static_cast<size_t>(LINES);
	menu_width = static_cast<size_t>(COLS);

	populate_items();
	menu_new(menu_height, menu_width, menu_starty, menu_startx, ctx);
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
	menu_print(2, static_cast<int>(state) + 4, menu_get_string(state)); // Start at row 4, indent from left border
	if (currentState == state)
	{
		menu_highlight_off();
	}
}

void MenuBuy::draw_content()
{
	// Only redraw if content changed
	populate_items();
	
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
	mvwprintw(menuWindow, 1, static_cast<int>(title_x), "Buy Items");
	
	// Instructions
	mvwprintw(menuWindow, 2, 2, "Use UP/DOWN or W/S to navigate, ENTER to buy, ESC to exit");
	
	populate_items();
	
	// Start items at row 4 to leave space for title and instructions
	for (size_t i{ 0 }; i < menuItems.size(); ++i)
	{
		menu_print_state(i);
	}
	menu_refresh();
}

void MenuBuy::on_key(int key, GameContext& ctx)
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
				handle_buy(menuWindow, buyer, *ctx.player);
				menu_mark_dirty(); // Redraw after purchase
			}
			else
			{
				ctx.message_system->message(WHITE_BLACK_PAIR, "No items for sale.", true);
			}
			break;
	}
}

void MenuBuy::menu(GameContext& ctx)
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
		on_key(keyPress, ctx);
	}
	
	// Clear screen when exiting
	clear();
	refresh();
}

void MenuBuy::handle_buy(WINDOW* tradeWin, Creature& shopkeeper_creature, Player& buyer)
{
    if (is_inventory_empty(shopkeeper.shop_inventory) ||
        currentState >= get_item_count(shopkeeper.shop_inventory))
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, "Invalid selection.", true);
        return;
    }

    Item* item = get_item_at(shopkeeper.shop_inventory, currentState);
    if (!item)
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, "Invalid selection.", true);
        return;
    }

    // Use ShopKeeper's purchase processing
    if (shopkeeper.process_player_purchase(ctx, *item, buyer))
    {
        // Remove item from shop inventory after successful purchase
        auto removed_item = remove_item_at(shopkeeper.shop_inventory, currentState);

        // Adjust currentState bounds
        if (currentState >= get_item_count(shopkeeper.shop_inventory) && !is_inventory_empty(shopkeeper.shop_inventory))
        {
            currentState = get_item_count(shopkeeper.shop_inventory) - 1;
        }

        // Refresh the display
        populate_items();
    }
    // Error messages are handled by process_player_purchase
}