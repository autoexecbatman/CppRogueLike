#pragma once

#include <string>

#include "BaseMenu.h"

struct GameContext;

class MenuName : public BaseMenu
{
	std::string inputText;

	void draw_name_screen(GameContext& ctx);

public:
	MenuName(GameContext& ctx);
	~MenuName();

	void menu_name(GameContext& ctx);
	void menu(GameContext& ctx) override;
};
