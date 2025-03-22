#pragma once

#include <vector>
#include <string>
#include <deque>
#include <span>

#include "BaseMenu.h"
#include "Actor/Actor.h"

class Item;
class Creature;
class Player;

class MenuBuy : public BaseMenu
{
	size_t menu_height; // set in constructor after items are populated for dynamic sizing
	size_t menu_width{ 20 };
	size_t menu_starty{ static_cast<size_t>((LINES / 2) - 5) };
	size_t menu_startx{ static_cast<size_t>((COLS / 2) - 10) };
	size_t currentState{ 0 };
	std::vector<std::string> menuItems;
	Creature& buyer;

	void populate_items(std::span<std::unique_ptr<Item>> item);
	void menu_print_state(size_t state);
	std::string menu_get_string(size_t state) { return menuItems.at(state); }
	void handle_buy(WINDOW* tradeWin, Creature& shopkeeper, Player& seller);
public:
	MenuBuy(Creature& buyer);
	~MenuBuy();

	void draw();
	void on_key(int key);
	void menu() override;
};