// file: Menu.h
#ifndef MENU_H
#define MENU_H

#include <iostream>
#include <curses.h>
#include <string>
#include <unordered_map>
#include <memory>

#include "../Game.h"
#include "../BaseMenu.h"
#include "../IMenuOptions.h"

class Menu : public BaseMenu
{
private:
	enum class MenuOptions : int
	{
		NONE,
		NEW_GAME,
		LOAD_GAME,
		OPTIONS,
		QUIT
	} currentOption{ MenuOptions::NEW_GAME };
	int newOption{ static_cast<int>(currentOption) };
public:
	std::unordered_map<MenuOptions, std::unique_ptr<IMenuOptions>> imenuOptions;

	int menu_height{ 10 };
	int menu_width{ 12 };
	int menu_starty{ (LINES / 2) - 5 };
	int menu_startx{ (COLS / 2) - 10 };

	std::string menu_get_string(MenuOptions option) { return menuOptions.at(option); }
	void menu_print_option(MenuOptions option);

public:
	std::unordered_map<MenuOptions, std::string> menuOptions
	{
		{ MenuOptions::NEW_GAME, "New Game" },
		{ MenuOptions::LOAD_GAME, "Load Game" },
		{ MenuOptions::OPTIONS, "Options" },
		{ MenuOptions::QUIT, "Quit" }
	};
	bool run{ true };

	Menu();
	~Menu();

	void menu() override;

	void menu_set_run_true() noexcept { run = true; }
	void menu_set_run_false() override { run = false; }
};

#endif // !MENU_H
// end of file: Menu.h
