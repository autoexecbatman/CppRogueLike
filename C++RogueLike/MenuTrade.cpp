#include "MenuTrade.h"
#include "Ai/AiPlayer.h"
#include "Game.h"
#include "MenuBuy.h"

void Buy::on_selection()
{
	game.menus.push_back(std::make_unique<MenuBuy>());
}

void Sell::on_selection()
{
	// do something
}

void Exit::on_selection()
{

}

MenuTrade::MenuTrade()
{
	menu_new(menu_height, menu_width, menu_starty, menu_startx);
	iMenuStates.emplace(MenuState::BUY, std::make_unique<Buy>());
	iMenuStates.emplace(MenuState::SELL, std::make_unique<Sell>());
	iMenuStates.emplace(MenuState::EXIT, std::make_unique<Exit>());
}

MenuTrade::~MenuTrade()
{
	menu_delete();
}

void MenuTrade::menu_print_state(MenuState state)
{
	auto row = static_cast<std::underlying_type_t<MenuState>>(state);
	if (currentState == state)
	{
		menu_highlight_on();
	}
	menu_print(1, row, menu_get_string(state));
	if (currentState == state)
	{
		menu_highlight_off();
	}
}

void MenuTrade::draw()
{
	menu_clear(); // clear menu window each frame
	mvwprintw(menuWindow, 0, 0, "%d", currentState); // this is for debugging the currentOption number
	// print the buttons to the menu window
	for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
	{
		menu_print_state(static_cast<MenuState>(i));
	}
	menu_refresh(); // refresh menu window each frame to show changes
}

void MenuTrade::on_key(int key)
{
	switch (key)
	{

	case KEY_UP:
	{
		currentState = static_cast<MenuState>((static_cast<size_t>(currentState) + iMenuStates.size() - 1) % iMenuStates.size());
		break; // break out of switch keep running menu loop
	}

	case KEY_DOWN:
	{
		currentState = static_cast<MenuState>((static_cast<size_t>(currentState) + 1) % iMenuStates.size());
		break; // break out of switch keep running menu loop
	}

	case 10:
	{ // if a selection is made
		menu_set_run_false();
		iMenuStates.at(currentState)->on_selection();
		break; // break out of switch keep running menu loop
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