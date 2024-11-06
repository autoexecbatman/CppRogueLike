#include <span>
#include <ranges>
#include <memory>

#include "MenuBuy.h"
#include "Game.h"
#include "ActorTypes/Player.h"

void MenuBuy::populate_items(std::span<std::unique_ptr<Item>> inventory)
{
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
	for (size_t i{ 0 }; i < buyer.container->inv.size(); ++i)
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
			currentState = (currentState + menuItems.size() - 1) % menuItems.size();
			break;
		case KEY_DOWN:
			currentState = (currentState + 1) % menuItems.size();
			break;
		case 27: // ESC
			menu_set_run_false();
			break;
		case 10: // ENTER
			// if selected buy item
			handle_buy(menuWindow, buyer, *game.player);
			break;
	}
}

void MenuBuy::menu()
{
	while (run)
	{
		draw();
		menu_key_listen();
		on_key(keyPress);
	}
}

void MenuBuy::handle_buy(WINDOW* tradeWin, Creature& shopkeeper, Player& seller)
{
	if (currentState < shopkeeper.container->inv.size())
	{
		size_t moduloSize = currentState % shopkeeper.container->inv.size();
		auto& item = shopkeeper.container->inv.at(moduloSize);

		if (seller.playerGold >= item->value) // Check if player has enough currency
		{
			seller.playerGold -= item->value; // Deduct currency
			seller.container->inv.insert(seller.container->inv.begin(), std::move(item)); // Transfer item from shopkeeper to player
			//shopkeeper.container->inv.erase(shopkeeper.container->inv.begin() + currentState); // Remove item from shopkeeper
			std::erase_if(shopkeeper.container->inv, [](const auto& item) { return item == nullptr; });
			game.message(WHITE_PAIR, "Item purchased successfully.", true);
		}
		else
		{
			game.message(WHITE_PAIR, "Insufficient currency.", true);
		}
	}
	else
	{
		game.message(WHITE_PAIR, "Invalid selection.", true);
	}

}