// file: MenuGender.h
#ifndef MENU_GENDER_H
#define MENU_GENDER_H

#include <iostream>
#include <string>

#include <curses.h>

#include "../BaseMenu.h"

class MenuGender : public BaseMenu
{
public:
	enum class MenuGenderOptions
	{
		NONE,
		MALE,
		FEMALE,
		RANDOM,
		BACK
	} currentGenderOption{ MenuGenderOptions::MALE };
private:

	int oldOption{ -1 };
	int newOption{ 1 };

	std::string gender{ "None" };
	int keyPress{ 0 };

	WINDOW* menuGenderWindow{ nullptr };
	int height_{ 10 };
	int width_{ 20 };
	int starty_{ (LINES / 2) - 5 };
	int startx_{ (COLS / 2) - 10 };

	void menu_gender_new(int height, int width, int starty, int startx) noexcept { menuGenderWindow = newwin(height, width, starty, startx); }
	void menu_gender_clear() noexcept;
	void menu_gender_print(int x, int y, const std::string& text) noexcept { mvwprintw(menuGenderWindow, y, x, text.c_str()); }
	void menu_gender_refresh() noexcept { wrefresh(menuGenderWindow); }
	void menu_gender_delete() noexcept { delwin(menuGenderWindow); }

	int menu_gender_get_oldOption() noexcept { return oldOption; }
	int menu_gender_get_newOption() noexcept { return newOption; }
	void menu_gender_set_oldOption(int option) noexcept { oldOption = option; }
	void menu_gender_set_newOption(int option) noexcept { newOption = option; }
	void menu_gender_highlight_on() noexcept { wattron(menuGenderWindow, A_REVERSE); }
	void menu_gender_highlight_off() noexcept { wattroff(menuGenderWindow, A_REVERSE); }
	void menu_gender_store(MenuGenderOptions option); // a function that will store the gender in a local variable
	void menu_gender_assign();
	void menu_gender_assign_random();

	void menu_gender_print_option(MenuGenderOptions option, int row) noexcept;
	std::string menu_gender_get_string(MenuGenderOptions option) noexcept;

	void key_listen() { std::clog << "getting key" << std::endl; keyPress = getch(); }

public:
	bool run{ true };

	//void menu() { /*dosomething*/ };
	void menu() override;
	void menu_gender_select();
	void menu_gender_set_run_true() noexcept { run = true; }
	void menu_set_run_false() override { run = false; }
};

#endif // !MENU_H
// file: MenuGender.h
