#ifndef MENU_H
#define MENU_H
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

	WINDOW* menuWindow{ nullptr };
	
	void menu_new(int height, int width, int starty, int startx) noexcept { menuWindow = newwin(height, width, starty, startx); }
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

public:
	void menu();
};

#endif // !MENU_H_