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
	ShopKeeper& shopkeeper; // Store shopkeeper reference
	GameContext& ctx; // Game context for message system

	void populate_items();
	void menu_print_state(size_t state);
	std::string menu_get_string(size_t state) { return menuItems.at(state); }
	void handle_buy();
	void draw_content() override;

public:
	MenuBuy(GameContext& ctx, Creature& buyer, ShopKeeper& shopkeeper);
	MenuBuy(const MenuBuy&) = delete;
	MenuBuy& operator=(const MenuBuy&) = delete;
	MenuBuy(MenuBuy&&) = delete;
	MenuBuy& operator=(MenuBuy&&) = delete;

	void draw();
	void on_key(int key, GameContext& ctx);
	void menu(GameContext& ctx) override;
};