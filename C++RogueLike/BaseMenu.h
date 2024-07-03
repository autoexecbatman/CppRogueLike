#pragma once
#include <curses.h>
#include <string>

class BaseMenu
{
protected:
	WINDOW* menuWindow{ nullptr };
	int menu_height{ 0 };
	int menu_width{ 0 };
	int menu_starty{ 0 };
	int menu_startx{ 0 };
	int keyPress{ 0 };
	bool run{ true };
public:
	BaseMenu() = default;
	virtual ~BaseMenu() = default;
	BaseMenu(const BaseMenu&) = delete;
	BaseMenu& operator=(const BaseMenu&) = delete;
	BaseMenu(BaseMenu&&) = delete;
	BaseMenu& operator=(BaseMenu&&) = delete;

	void menu_new(int height, int width, int starty, int startx);
	void menu_clear() { wclear(menuWindow); };
	void menu_print(int x, int y, const std::string& text) { mvwprintw(menuWindow, y, x, text.c_str()); };
	void menu_refresh() { wrefresh(menuWindow); };
	void menu_delete() { delwin(menuWindow); };
	void menu_highlight_on() { wattron(menuWindow, A_REVERSE); };
	void menu_highlight_off() { wattroff(menuWindow, A_REVERSE); };
	void menu_key_listen() { keyPress = getch(); };
	void menu_set_run_true() { run = true; };
	void menu_set_run_false() { run = false; };

	virtual void menu() = 0;
};
