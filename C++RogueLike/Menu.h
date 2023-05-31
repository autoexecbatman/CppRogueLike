// file: Menu.h
#ifndef MENU_H
#define MENU_H
#include <iostream>
#include <curses.h>
#include <string>

class Menu
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

	int oldOption{ -1 };
	int newOption{ 1 };
	bool run{ true };
	int keyPress{ 0 };

	WINDOW* menuWindow{ nullptr };
	int menu_height{ 10 };
	const int menu_width{ 12 };
	int menu_starty{ (LINES / 2) - 5 };
	int menu_startx{ (COLS / 2) - 10 };

	void menu_new(int height, int width, int starty, int startx);
	void menu_clear() noexcept { wclear(menuWindow); }
	void menu_print(int x, int y, const std::string& text) noexcept { mvwprintw(menuWindow, y, x, text.c_str()); }
	void menu_refresh() noexcept { wrefresh(menuWindow); }
	void menu_delete() noexcept { delwin(menuWindow); }
	void menu_highlight_on() noexcept { wattron(menuWindow, A_REVERSE); }
	void menu_highlight_off() noexcept { wattroff(menuWindow, A_REVERSE); }

	void menu_set_run_true() noexcept { run = true; }
	void menu_set_run_false() noexcept { run = false; }
	int menu_get_oldOption() noexcept { return oldOption; }
	int menu_get_newOption() noexcept { return newOption; }
	void menu_set_oldOption(int option) noexcept { oldOption = option; }
	void menu_set_newOption(int option) noexcept { newOption = option; }

	std::string menu_get_string(MenuOptions option) noexcept;
	void menu_print_option(MenuOptions option, int row) noexcept;

	void key_listen() { std::clog << "getting key" << std::endl; keyPress = getch(); }

public:
	Menu() : menuWindow(nullptr),menu_height(10), menu_width(12), menu_starty((LINES / 2) - 5), menu_startx((COLS / 2) - 10) {}
	void menu();
};

#endif // !MENU_H
// end of file: Menu.h
