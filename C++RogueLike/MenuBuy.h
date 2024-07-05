#pragma once
#include <vector>

#include "BaseMenu.h"

class MenuBuy : public BaseMenu
{
	int menu_height{ static_cast<int>(menuItems.size())};
	int menu_width{ 20 };
	int menu_starty{ (LINES / 2) - 5 };
	int menu_startx{ (COLS / 2) - 10 };
	size_t currentState{ 0 };

	std::vector<std::string> menuItems;
	void populate_items();
	void menu_print_state(size_t state);
	std::string menu_get_string(size_t state) { return menuItems.at(state); }
public:
	MenuBuy();
	~MenuBuy();

	void draw();
	void on_key(int key);
	void menu() override;
};