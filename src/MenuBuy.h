#pragma once

#include <string>
#include <vector>

#include "BaseMenu.h"

struct GameContext;
class Item;
class Creature;
class ShopKeeper;

class MenuBuy : public BaseMenu
{
	size_t currentState{ 0 };
	std::vector<std::string> menuItems;
	Creature& buyer;
	// The shopkeeper creature, whose purse takes the price. Declared before the shop it
	// owns, which is derived from it.
	Creature& owner;
	ShopKeeper& shopkeeper;

	void populate_items();
	void menu_print_state(size_t state);

	std::string menu_get_string(size_t state) { return menuItems.at(state); }

	// Buys the item under the cursor. Takes the frame's context: Game::tick builds a new
	// one each frame, so a menu that kept the one it was constructed with would outlive it.
	void handle_buy(GameContext& ctx);
	void draw_content() override;

public:
	MenuBuy(GameContext& ctx, Creature& buyer, Creature& owner);
	MenuBuy(const MenuBuy&) = delete;
	MenuBuy& operator=(const MenuBuy&) = delete;
	MenuBuy(MenuBuy&&) = delete;
	MenuBuy& operator=(MenuBuy&&) = delete;

	void draw();
	void on_key(GameContext& ctx) override;
	void menu(GameContext& ctx) override;
};