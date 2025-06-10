// file: MenuRace.h
#ifndef MENU_RACE_H
#define MENU_RACE_H
#include <curses.h>
#include <string>
#include <unordered_map>
#include <memory>

#include "BaseMenu.h"
#include "IMenuState.h"

class Human : public IMenuState
{
	void on_selection() override;
};

class Dwarf : public IMenuState
{
	void on_selection() override;
};

class Elf : public IMenuState
{
	void on_selection() override;
};

class Gnome : public IMenuState
{
	void on_selection() override;
};

class HalfElf : public IMenuState
{
	void on_selection() override;
};

class Halfling : public IMenuState
{
	void on_selection() override;
};

class RaceRandom : public IMenuState
{
	void on_selection() override;
};

class RaceBack : public IMenuState
{
	void on_selection() override;
};

class MenuRace : public BaseMenu
{
	int height_{ 10 };
	int width_{ 20 };
	int starty_{ (LINES / 2) - 5 };
	int startx_{ (COLS / 2) - 10 };
	enum class MenuRaceOptions
	{
		HUMAN, DWARF, ELF, GNOME, HALFELF, HALFLING, RANDOM, BACK
	}
	currentState{ MenuRaceOptions::HUMAN };
	std::unordered_map<MenuRaceOptions, std::unique_ptr<IMenuState>> iMenuStates;
	std::unordered_map<MenuRaceOptions, std::string> menuRaceStrings
	{
		{ MenuRaceOptions::HUMAN, "Human" },
		{ MenuRaceOptions::DWARF, "Dwarf" },
		{ MenuRaceOptions::ELF, "Elf" },
		{ MenuRaceOptions::GNOME, "Gnome" },
		{ MenuRaceOptions::HALFELF, "Half-Elf" },
		{ MenuRaceOptions::HALFLING, "Halfling" },
		{ MenuRaceOptions::RANDOM, "Random" },
		{ MenuRaceOptions::BACK, "Back" }
	};

	std::string menu_race_get_string(MenuRaceOptions option) { return menuRaceStrings.at(option); }
	void menu_race_print_option(MenuRaceOptions option) noexcept;

public:
	MenuRace();
	~MenuRace();

	void draw();
	void menu() override;
	void on_key(int key);
};

#endif // !MENU_RACE_H
// end of file: MenuRace.h
