// file: MenuGender.h
#ifndef MENU_GENDER_H
#define MENU_GENDER_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <curses.h>

#include "../BaseMenu.h"
#include "../IMenuState.h"

class IMenuState;

class Male : public IMenuState
{
	void on_selection() override;
};

class Female : public IMenuState
{
	void on_selection() override;
};

class Random : public IMenuState
{
	void on_selection() override;
};

class Back : public IMenuState
{
	void on_selection() override;
};

class MenuGender : public BaseMenu
{
	int height_{ 10 };
	int width_{ 20 };
	int starty_{ (LINES / 2) - 5 };
	int startx_{ (COLS / 2) - 10 };
	enum class MenuState { MALE, FEMALE, RANDOM, BACK }
	stateEnum{ MenuState::MALE };
	int stateInt{ static_cast<int>(stateEnum) };
	std::unordered_map<MenuState, std::unique_ptr<IMenuState>> iMenuStates;
	std::unordered_map<MenuState, std::string> menuStateStrings
	{
		{ MenuState::MALE, "Male"},
		{ MenuState::FEMALE, "Female"},
		{ MenuState::RANDOM, "Random"},
		{ MenuState::BACK, "Back"}
	};

	std::string menu_gender_get_string(MenuState option) { return menuStateStrings.at(option); }
	void menu_print_state(MenuState option);
public:
	MenuGender();
	~MenuGender();

	void menu() override;
};

#endif // !MENU_H
// file: MenuGender.h
