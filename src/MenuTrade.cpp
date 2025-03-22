#include "MenuTrade.h"
#include "Ai/AiPlayer.h"
#include "Game.h"
#include "MenuBuy.h"
#include "MenuSell.h"
#include "Actor/Actor.h"

void Buy::on_selection()
{
	game.menus.push_back(std::make_unique<MenuBuy>(shopkeeper));
}

void Sell::on_selection()
{
	game.menus.push_back(std::make_unique<MenuSell>(shopkeeper, player));
}

void Exit::on_selection()
{
	
}

MenuTrade::MenuTrade(Creature& shopkeeper, Creature& player)
{
	menu_new(height_, width_, starty_, startx_);
	iMenuStates.push_back(std::make_unique<Buy>(shopkeeper));
	iMenuStates.push_back(std::make_unique<Sell>(shopkeeper, player));
	iMenuStates.push_back(std::make_unique<Exit>());
}

MenuTrade::~MenuTrade()
{
	menu_delete();
}

void MenuTrade::menu_print_state(size_t state)
{
	if (currentState == state)
	{
		menu_highlight_on();
		menu_print(1, state, menu_get_string(state));
		menu_highlight_off();
	}
	else
	{
		menu_print(1, state, menu_get_string(state));
	}
}

void MenuTrade::draw()
{
	menu_clear();
	mvwprintw(menuWindow, 0, 0, "%d", currentState); // for debugging `currentState`
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
	{
		currentState = (currentState + iMenuStates.size() - 1) % iMenuStates.size();
		break;
	}

	case KEY_DOWN:
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
	}

	default:
		break;
	}
}

void MenuTrade::menu()
{
	while (run)
	{
		draw();
		menu_key_listen();
		on_key(keyPress);
	}
}