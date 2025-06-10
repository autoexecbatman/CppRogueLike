#include <span>
#include <ranges>
#include <memory>

#include "MenuBuy.h"
#include "../Game.h"
#include "../ActorTypes/Player.h"

void MenuBuy::populate_items(std::span<std::unique_ptr<Item>> inventory)
{
	menuItems.clear();
	for (const auto& item : inventory)
	{
		if (item)
		{
			menuItems.push_back(item->actorData.name);
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
	populate_items(buyer.container->inv); // must first populate items to get the size of the menu
	menu_height = menuItems.size();
	menu_new( menu_height, menu_width, menu_starty, menu_startx );
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
	menu_print(1, state, menu_get_string(state));
	if (currentState == state)
	{
		menu_highlight_off();
	}
}

void MenuBuy::draw()
{
	// print items to buy from shopkeeper
	menu_clear();
	populate_items(buyer.container->inv);
	for (size_t i{ 0 }; i < menuItems.size(); ++i) // Use menuItems.size() not inv.size()
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
			if (menuItems.empty())
			{
				return;
			}
			currentState = (currentState + menuItems.size() - 1) % menuItems.size();
			break;
		case KEY_DOWN:
		case 's':
			if (menuItems.empty())
			{
				return;
			}
			currentState = (currentState + 1) % menuItems.size();
			break;
		case 27: // ESC
			menu_set_run_false();
			break;
		case 10: // ENTER
			if (!menuItems.empty()) // Only handle if menu has items
			{
				handle_buy(menuWindow, buyer, *game.player);
			}
			break;
	}
}

void MenuBuy::menu()
{
	while (run)
	{
		clear();
		game.render();
		refresh();
		draw();
		menu_key_listen();
		on_key(keyPress);
	}
	clear();
	refresh();
}
void MenuBuy::handle_buy(WINDOW* tradeWin, Creature& shopkeeper, Player& seller)
{
	if (shopkeeper.container->inv.empty() || currentState >= shopkeeper.container->inv.size())
	{
		game.message(WHITE_PAIR, "Invalid selection.", true);
		return;
	}

	size_t index = currentState % shopkeeper.container->inv.size();
	auto itemIter = shopkeeper.container->inv.begin() + index;
	auto& item = *itemIter;

	if (seller.gold >= item->value) // Check if player has enough currency
	{
		seller.gold -= item->value; // Deduct currency
		shopkeeper.gold += item->value; // Add currency
		seller.container->inv.insert(seller.container->inv.begin(), std::move(item)); // Transfer item

		// Properly erase item from shopkeeper's inventory
		shopkeeper.container->inv.erase(itemIter);

		game.message(WHITE_PAIR, "Item purchased successfully.", true);
	}
	else
	{
		game.message(WHITE_PAIR, "Insufficient currency.", true);
	}
}