#pragma once

#include "BaseMenu.h"


class MenuBuy : public BaseMenu
{
	int menu_height{ 3 };
	int menu_width{ 12 };
	int menu_starty{ (LINES / 2) - 5 };
	int menu_startx{ (COLS / 2) - 10 };

	// get shopkeeper to display items
public:
	MenuBuy();
	~MenuBuy();

	void draw();
	void on_key(int key);
	void menu() override;
};