#include "MenuTrade.h"
#include "../Ai/AiPlayer.h"
#include "../Ai/AiShopkeeper.h"  // MISSING INCLUDE
#include "../Game.h"
#include "MenuBuy.h"
#include "MenuSell.h"
#include "../Actor/Actor.h"
#include "../Systems/ShopKeeper.h"

void Buy::on_selection()
{
	// Use the shopkeeper's attached shop component
	if (shopkeeper.shop != nullptr)
	{
		game.menus.push_back(std::make_unique<MenuBuy>(*game.player, *shopkeeper.shop));
	}
	else
	{
		game.log("Error: Shopkeeper has no shop component!");
	}
}

void Sell::on_selection()
{
	game.menus.push_back(std::make_unique<MenuSell>(shopkeeper, player));
}

void Exit::on_selection()
{
}

MenuTrade::MenuTrade(Creature& shopkeeper, Creature& player) : shopkeeper(shopkeeper)
{
	menu_new(height_, width_, starty_, startx_);
	iMenuStates.push_back(std::make_unique<Buy>(shopkeeper));
	iMenuStates.push_back(std::make_unique<Sell>(shopkeeper, player));
	iMenuStates.push_back(std::make_unique<Exit>());
}

MenuTrade::~MenuTrade()
{
	menu_delete();
	
	// CRITICAL FIX: Reset shopkeeper trade state when menu closes
	if (shopkeeper.ai)
	{
		auto* shopAi = dynamic_cast<AiShopkeeper*>(shopkeeper.ai.get());
		if (shopAi)
		{
			shopAi->tradeMenuOpen = false;
		}
	}
}

void MenuTrade::menu_print_state(size_t state)
{
	if (currentState == state)
	{
		menu_highlight_on();
		menu_print(1, state + 1, menu_get_string(state)); // Start at row 1 after title
		menu_highlight_off();
	}
	else
	{
		menu_print(1, state + 1, menu_get_string(state)); // Start at row 1 after title
	}
}

void MenuTrade::draw_content()
{
	// Debug current state
	mvwprintw(menuWindow, 0, 0, "%d", currentState);
	
	// Draw menu options
	for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
	{
		menu_print_state(i);
	}
}

void MenuTrade::draw()
{
	menu_clear();
	box(menuWindow, 0, 0);
	// Title
	mvwprintw(menuWindow, 0, 1, "Trade");
	for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
	{
		menu_print_state(i);
	}
	menu_refresh();
}

void MenuTrade::on_key(int key)
{
	switch (key)
	{

	case KEY_UP:
	case 'w':
	{
		currentState = (currentState + iMenuStates.size() - 1) % iMenuStates.size();
		break;
	}

	case KEY_DOWN:
	case 's':
	{
		currentState = (currentState + 1) % iMenuStates.size();
		break;
	}

	case 10:
	{ // if a selection is made
		menu_set_run_false();
		iMenuStates.at(currentState)->on_selection();
		break;
	}

	case 27:
	{
		menu_set_run_false();
		break;
	}

	default:
		break;
	}
}

void MenuTrade::menu()
{
	// Save background before showing menu
	clear();
	game.render();
	refresh();
	
	while (run)
	{
		draw();
		menu_key_listen();
		on_key(keyPress);
	}
	// Restore full game view
	clear();
	game.render();
	refresh();
}