#pragma once

#include <string>

#include "BaseMenu.h"

struct GameContext;

class MenuName : public BaseMenu
{
	std::string inputText;
	bool initialized{ false };

	void draw_name_screen(GameContext& ctx);

public:
	MenuName(GameContext& ctx);
	~MenuName();
	MenuName(const MenuName&) = delete;
	MenuName& operator=(const MenuName&) = delete;
	MenuName(MenuName&&) = delete;
	MenuName& operator=(MenuName&&) = delete;

	void menu_name(GameContext& ctx);
	void menu(GameContext& ctx) override;
};
