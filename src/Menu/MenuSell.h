#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>

#include "../Actor/Actor.h"
#include "../Core/GameContext.h"
#include "BaseMenu.h"

class Player;

class MenuSell : public BaseMenu
{
	size_t currentState{ 0 };
	Creature& player;
	Creature& shopkeeper;
	std::vector<std::string> menuItems;

	void populate_items(std::span<std::unique_ptr<Item>> item);
	void menu_print_state(size_t state);
	std::string menu_get_string(size_t state) { return menuItems.at(state); }
	void handle_sell(void* tradeWin, Creature& shopkeeper, Creature& seller, GameContext& ctx);
	void draw_content() override;

public:
	MenuSell(Creature& shopkeeper, Creature& player, GameContext& ctx);
	MenuSell(const MenuSell&) = delete;
	MenuSell& operator=(const MenuSell&) = delete;
	MenuSell(MenuSell&&) = delete;
	MenuSell& operator=(MenuSell&&) = delete;

	void draw();
	void on_key(int key, GameContext& ctx);
	void menu(GameContext& ctx) override;
};