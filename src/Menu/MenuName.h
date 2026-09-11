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
	// Pixels available for text between the frame's left and right borders, which
	// is the box less one tile of border on each side. The field measures itself
	// against this before accepting a keystroke, so a name can never draw past the
	// frame however wide the glyphs are.
	//
	// Example, at the 1280x896 viewport where the box settles at 16 tiles:
	//
	//   interior_width();   // -> 896, from (16 - 2) tiles at 64 pixels
	//
	// Typing fifty W characters fills the field and stops, leaving the cursor
	// visible inside the border.
	[[nodiscard]] int interior_width() const;

public:
	MenuName(GameContext& ctx);
	MenuName(const MenuName&) = delete;
	MenuName& operator=(const MenuName&) = delete;
	MenuName(MenuName&&) = delete;
	MenuName& operator=(MenuName&&) = delete;

	void menu(GameContext& ctx) override;
};
