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

	//enum class MenuState : size_t { BUY, SELL, EXIT }
	//currentState{ MenuState::BUY };
	size_t currentState{ 0 };
	std::vector<std::unique_ptr<IMenuState>> iMenuStates;
	std::vector<std::string> menuStateStrings{ "Buy","Sell","Exit" };

	std::unordered_map<size_t, std::function<void()>> menuCallbacks
	{
		{ 0, [&] { iMenuStates.at(currentState)->on_selection(); } },
		{ 1, [&] { iMenuStates.at(currentState)->on_selection(); } },
		{ 2, [&] { iMenuStates.at(currentState)->on_selection(); } }
	};

	std::string menu_get_string(size_t state) { return menuStateStrings.at(state); }
	void menu_print_state(size_t state);

public:
	MenuTrade(Creature& shopkeeper, Player& player);
	~MenuTrade();

	void draw();
	void on_key(int key);
	void menu() override;
};
