#pragma once

#include <functional>
#include <string>
#include <memory>
#include <unordered_map>

#include "BaseMenu.h"
#include "IMenuState.h"
#include "../Actor/Actor.h"
#include "../ActorTypes/Player.h"
#include "../Core/GameContext.h"

struct GameContext;

class Buy : public IMenuState
{
	void on_selection(GameContext& ctx) override;
	Creature& shopkeeper;
public:
	Buy(Creature& shopkeeper) : shopkeeper{ shopkeeper } {}
};

class Sell : public IMenuState
{
	void on_selection(GameContext& ctx) override;
	Creature& player;
	Creature& shopkeeper;
public:
	Sell(Creature& shopkeeper, Creature& seller) : player{ seller }, shopkeeper{ shopkeeper } {}
};

class Exit : public IMenuState
{
    void on_selection(GameContext& ctx) override;
};

class MenuTrade : public BaseMenu
{
	int height_{ 5 }; // Title row + 3 menu items + 2 border rows + spacing
	int width_{ 10 }; // Wide enough for "Trade" title
	int starty_{ 0 };
	int startx_{ 0 };
	Creature& shopkeeper;  // Store reference to shopkeeper

	size_t currentState{ 0 };
	std::vector<std::unique_ptr<IMenuState>> iMenuStates;
	std::vector<std::string> menuStateStrings{ "Buy","Sell","Exit" };

	std::unordered_map<size_t, std::function<void(GameContext& ctx)>> menuCallbacks
	{
		{ 0, [&](GameContext& ctx) { iMenuStates.at(currentState)->on_selection(ctx); } },
		{ 1, [&](GameContext& ctx) { iMenuStates.at(currentState)->on_selection(ctx); } },
		{ 2, [&](GameContext& ctx) { iMenuStates.at(currentState)->on_selection(ctx); } }
	};

	std::string menu_get_string(size_t state) { return menuStateStrings.at(state); }
	void menu_print_state(size_t state);
	void draw_content() override;

public:
	MenuTrade(Creature& shopkeeper, Creature& player, GameContext& ctx);
	~MenuTrade();

	void draw();
	void on_key(int key, GameContext& ctx);
	void menu(GameContext& ctx) override;
};
