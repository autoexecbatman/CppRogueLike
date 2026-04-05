#pragma once

#include <functional>
#include <string>
#include <vector>

#include "BaseMenu.h"

struct GameContext;

class ContextMenu : public BaseMenu
{
public:
	ContextMenu(
		std::vector<std::string> options,
		int anchor_col,
		int anchor_row,
		std::function<void(int, GameContext&)> callback,
		GameContext& ctx);

	// Called once per frame by MenuManager -- no blocking loop
	void menu(GameContext& ctx) override;

private:
	std::vector<std::string> menuOptions;
	int selectedIndex{ 0 };
	std::function<void(int, GameContext&)> onSelect;
	void draw_content() override;
};
