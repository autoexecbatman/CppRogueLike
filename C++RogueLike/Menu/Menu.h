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

	int keyPress{ 0 };

	WINDOW* menuWindow{ nullptr };
	int menu_height{ 10 };
	const int menu_width{ 12 };
	int menu_starty{ (LINES / 2) - 5 };
	int menu_startx{ (COLS / 2) - 10 };

	void menu_new(int height, int width, int starty, int startx);
	void menu_clear();
	void menu_print(int x, int y, const std::string& text);
	void menu_refresh();
	void menu_delete();
	void menu_highlight_on();
	void menu_highlight_off();

	int menu_get_oldOption() noexcept { return oldOption; }
	int menu_get_newOption() noexcept { return newOption; }
	void menu_set_oldOption(int option) noexcept { oldOption = option; }
	void menu_set_newOption(int option) noexcept { newOption = option; }

	std::string menu_get_string(MenuOptions option) noexcept;
	void menu_print_option(MenuOptions option, int row);

	void menu_key_listen();

public:
	bool run{ true };

	Menu() noexcept {}
	void menu();
	void menu_set_run_true() noexcept { run = true; }
	void menu_set_run_false() noexcept { run = false; }
};

#endif // !MENU_H
// end of file: Menu.h
