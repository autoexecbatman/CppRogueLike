#pragma once

#include <curses.h>
#include <string>
#include <memory>
#include <unordered_map>

#include "BaseMenu.h"
#include "IMenuState.h"
#include "Actor/Actor.h"
#include "ActorTypes/Player.h"

class Buy : public IMenuState
{
	void on_selection() override;
	Creature& shopkeeper;
public:
	Buy(Creature& shopkeeper) : shopkeeper{ shopkeeper } {}
};

class Sell : public IMenuState
{
	void on_selection() override;
	Player& player;
	Creature& shopkeeper;
public:
	Sell(Creature& shopkeeper, Player& seller) : player{ seller }, shopkeeper{ shopkeeper } {}
};

class Exit : public IMenuState
{
	void on_selection() override;
};

class MenuTrade : public BaseMenu
{
	int height_{ 3 };
	int width_{ 12 };
	int starty_{ (LINES / 2) - 5 };
	int startx_{ (COLS / 2) - 10 };

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
	MenuTrade(Creature& shopkeeper, Player& player);
	~MenuTrade();

	void draw();
	void on_key(int key);
	void menu() override;
};
