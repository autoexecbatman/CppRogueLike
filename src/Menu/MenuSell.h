#pragma once

#include <string>
#include <span>
#include <vector>

#include "BaseMenu.h"
#include "../Actor/Actor.h"

class Player;

class MenuSell : public BaseMenu
{
	size_t menu_height; // Full screen height
	size_t menu_width; // Full screen width
	size_t menu_starty{ 0 }; // Full screen - start at top
	size_t menu_startx{ 0 }; // Full screen - start at left
	size_t currentState{ 0 };
	std::vector<std::string> menuItems;
	Creature& player;
	Creature& shopkeeper;

	void populate_items(std::span<std::unique_ptr<Item>> item);
	void menu_print_state(size_t state);
	std::string menu_get_string(size_t state) { return menuItems.at(state); }
	void handle_sell(WINDOW* tradeWin, Creature& shopkeeper, Creature& seller, GameContext& ctx);
	void draw_content() override;
public:
	MenuSell(Creature& shopkeeper, Creature& player, GameContext& ctx);
	~MenuSell();

	void draw();
	void on_key(int key, GameContext& ctx);
	void menu(GameContext& ctx) override;
};