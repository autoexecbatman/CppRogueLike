#pragma once

#include <vector>
#include <string>

#include "BaseMenu.h"

struct GameContext;
class Item;
class Creature;
class Player;
class ShopKeeper;

class MenuBuy : public BaseMenu
{
	size_t menu_height{ 0 };
	size_t menu_width{ 0 };
	size_t menu_starty{ 0 }; // Full screen - start at top
	size_t menu_startx{ 0 }; // Full screen - start at left
	size_t currentState{ 0 };
	std::vector<std::string> menuItems;
	Creature& buyer;
	ShopKeeper& shopkeeper; // Store shopkeeper reference
	GameContext& ctx; // Game context for message system

	void populate_items();
	void menu_print_state(size_t state);
	std::string menu_get_string(size_t state) { return menuItems.at(state); }
	void handle_buy(void* tradeWin, Creature& shopkeeper, Player& seller);
	void draw_content();
public:
	MenuBuy(GameContext& ctx, Creature& buyer, ShopKeeper& shopkeeper);
	~MenuBuy();

	void draw();
	void on_key(int key, GameContext& ctx);
	void menu(GameContext& ctx) override;
};