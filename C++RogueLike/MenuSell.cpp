#include "MenuSell.h"
#include "Game.h"
#include "Actor/Actor.h"
#include "BaseMenu.h"

void MenuSell::populate_items(std::span<std::unique_ptr<Item>> item)
{
		for (const auto& item : item)
	{
		if (item)
		{
			menuItems.push_front(item->actorData.name);
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

void MenuSell::handle_sell(WINDOW* tradeWin, Creature& shopkeeper, Player& seller)
{
	auto& item = seller.container->inv.at(currentState);
	if (item)
	{
		auto price = item->value;
		if (seller.playerGold >= price)
		{
			seller.playerGold -= price;
			seller.playerGold += price;
			shopkeeper.container->inv.push_back(std::move(item));
			seller.container->inv.erase(seller.container->inv.begin() + currentState);
		}
		else
		{
			game.log("You don't have enough gold to buy that.");
		}
	}
	else
	{
		game.log("Item is null.");
		std::exit(EXIT_FAILURE);
	}

}

MenuSell::MenuSell(Creature& shopkeeper, Player& player) : player(player), shopkeeper(shopkeeper)
{
	populate_items(player.container->inv); // must first populate items to get the size of the menu
	menu_height = menuItems.size();
	menu_new(menu_height, menu_width, menu_starty, menu_startx);
}

MenuSell::~MenuSell()
{
	menu_delete();
}

void MenuSell::draw()
{
	menu_clear();
	for (size_t i{ 0 }; i < player.container->inv.size(); ++i)
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
		if (currentState > 0)
		{
			--currentState;
		}
		break;
	case KEY_DOWN:
		if (currentState < menuItems.size() - 1)
		{
			++currentState;
		}
		break;
	case 10: // Enter key
	{
		 handle_sell(menuWindow, shopkeeper, player);
	}
	break;
	case 27: // Escape key
	{
		run = false;
	}
	break;
	}
}

void MenuSell::menu()
{
	while(run)
	{
		draw();
		on_key(getch());
	}
}
