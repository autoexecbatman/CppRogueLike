#pragma once

#include <string>

#include "BaseMenu.h"

struct GameContext;

class MenuName : public BaseMenu
{
private:
	std::string inputText{};
	bool initialized{ false };

	void draw_name_screen();
	[[nodiscard]] int interior_width() const;

public:
	MenuName(GameContext& ctx);
	MenuName(const MenuName&) = delete;
	MenuName& operator=(const MenuName&) = delete;
	MenuName(MenuName&&) = delete;
	MenuName& operator=(MenuName&&) = delete;

	void menu(GameContext& ctx) override;
};
