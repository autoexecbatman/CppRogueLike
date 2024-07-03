// file: MenuGender.h
#ifndef MENU_GENDER_H
#define MENU_GENDER_H

#include <iostream>
#include <string>

#include <curses.h>

#include "../BaseMenu.h"

class MenuGender : public BaseMenu
{
	enum class MenuGenderOptions
	{
		NONE,
		MALE,
		FEMALE,
		RANDOM,
		BACK
	} currentGenderOption{ MenuGenderOptions::MALE };
private:
	int newState{ 1 };

	std::string gender{ "None" };
	int keyPress{ 0 };

	int height_{ 10 };
	int width_{ 20 };
	int starty_{ (LINES / 2) - 5 };
	int startx_{ (COLS / 2) - 10 };

	void menu_gender_store(MenuGenderOptions option); // a function that will store the gender in a local variable
	void menu_gender_assign();
	void menu_gender_assign_random();

	void menu_gender_print_option(MenuGenderOptions option, int row) noexcept;
	std::string menu_gender_get_string(MenuGenderOptions option) noexcept;

public:
	/*bool run{ true };*/

	void menu() override;
	void menu_gender_select();
	void menu_gender_set_run_true() noexcept { run = true; }
	/*void menu_set_run_false() override { run = false; }*/
};

#endif // !MENU_H
// file: MenuGender.h
