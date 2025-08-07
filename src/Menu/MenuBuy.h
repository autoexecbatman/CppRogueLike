#pragma once

#include <vector>
#include <string>
#include <deque>
#include <span>

#include "BaseMenu.h"
#include "../Actor/Actor.h"

class Item;
class Creature;
class Player;

class MenuBuy : public BaseMenu
{
	size_t menu_height; // Full screen height
	size_t menu_width; // Full screen width
	size_t menu_starty{ 0 }; // Full screen - start at top
	size_t menu_startx{ 0 }; // Full screen - start at left
	size_t currentState{ 0 };
	std::vector<std::string> menuItems;
	Creature& buyer;

	void populate_items(std::span<std::unique_ptr<Item>> item);
	void menu_print_state(size_t state);
	std::string menu_get_string(size_t state) { return menuItems.at(state); }
	void handle_buy(WINDOW* tradeWin, Creature& shopkeeper, Player& seller);
	void draw_content() override;
public:
	MenuBuy(Creature& buyer);
	~MenuBuy();

	void draw();
	void on_key(int key);
	void menu() override;
};