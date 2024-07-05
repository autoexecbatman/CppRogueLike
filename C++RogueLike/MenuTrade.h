#pragma once

#include <curses.h>
#include <string>
#include <memory>
#include <unordered_map>

#include "BaseMenu.h"
#include "IMenuState.h"

class Buy : public IMenuState
{
	void on_selection() override;
};

class Sell : public IMenuState
{
	void on_selection() override;
};

class Exit : public IMenuState
{
	void on_selection() override;
};

class MenuTrade : public BaseMenu
{
	int menu_height{ 3 };
	int menu_width{ 12 };
	int menu_starty{ (LINES / 2) - 5 };
	int menu_startx{ (COLS / 2) - 10 };

	enum class MenuState { BUY, SELL, EXIT }
	currentState{ MenuState::BUY };
	std::unordered_map<MenuState, std::unique_ptr<IMenuState>> iMenuStates;
	std::unordered_map<MenuState, std::string> menuStateStrings
	{
		{ MenuState::BUY, "Buy" },
		{ MenuState::SELL, "Sell" },
		{ MenuState::EXIT, "Exit" }
	};

	std::string menu_get_string(MenuState state) { return menuStateStrings.at(state); }
	void menu_print_state(MenuState state);

public:
	MenuTrade();
	~MenuTrade();

	void draw();
	void on_key(int key);
	void menu() override;
};
