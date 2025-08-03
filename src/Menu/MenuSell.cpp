#include "MenuSell.h"
#include "../Game.h"
#include "../Actor/Actor.h"
#include "BaseMenu.h"

void MenuSell::populate_items(std::span<std::unique_ptr<Item>> item)
{
	menuItems.clear();
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
	menu_print(1, state, menu_get_string(state));
	if (currentState == state)
	{
		menu_highlight_off();
	}
}

void MenuSell::handle_sell(WINDOW* tradeWin, Creature& shopkeeper, Creature& seller)
{
	if (seller.container->inv.empty() || currentState >= seller.container->inv.size())
	{
		game.message(WHITE_BLACK_PAIR, "Invalid selection.", true);
		return;
	} 

	auto itemIter = seller.container->inv.begin() + currentState;
	auto& item = *itemIter;

	if (!item)
	{
		game.log("Error: Attempted to sell a null item.");
		game.message(WHITE_BLACK_PAIR, "Error: Invalid item.", true);
		return;
	}

	auto price = item->value;
	if (shopkeeper.gold >= price)
	{
		shopkeeper.gold -= price;
		seller.gold += price;
		shopkeeper.container->inv.push_back(std::move(item));
		seller.container->inv.erase(itemIter);

		// Ensure currentState stays within valid bounds
		if (currentState >= seller.container->inv.size() && !seller.container->inv.empty())
		{
			currentState = seller.container->inv.size() - 1;
		}

		game.message(WHITE_BLACK_PAIR, "Item sold successfully.", true);
	}
	else
	{
		game.log("Shopkeeper does not have enough gold to buy the item.");
		game.message(WHITE_BLACK_PAIR, "Shopkeeper does not have enough gold to buy the item.", true);
	}
}

MenuSell::MenuSell(Creature& shopkeeper, Creature& player) : player(player), shopkeeper(shopkeeper)
{
	populate_items(player.container->inv); // must first populate items to get the size of the menu
	menu_height = menuItems.size();
	menu_new(menu_height, menu_width, menu_starty, menu_startx);
}

MenuSell::~MenuSell()
{
	menu_delete();
}

void MenuSell::draw_content()
{
	// Only redraw if content changed
	populate_items(player.container->inv);
	
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
	// Title
	mvwprintw(menuWindow, 0, 1, "Sell Items");
	populate_items(player.container->inv);
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
		if (menuItems.empty()) return; // Check for empty menu
		currentState = (currentState + menuItems.size() - 1) % menuItems.size();
		menu_mark_dirty(); // Mark for redraw
		break;
	case KEY_DOWN:
	case 's':
		if (menuItems.empty()) return; // Check for empty menu
		currentState = (currentState + 1) % menuItems.size();
		menu_mark_dirty(); // Mark for redraw
		break;
	case 10: // Enter key
	{
		if (!menuItems.empty()) // Only handle if menu has items
		{
			handle_sell(menuWindow, shopkeeper, player);
			menu_mark_dirty(); // Redraw after sale
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
