#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include <curses.h>

#include "BaseMenu.h"
#include "IMenuState.h"

struct GameContext;

class Human : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class Dwarf : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class Elf : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class Gnome : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class HalfElf : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class Halfling : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class RaceRandom : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class RaceBack : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class MenuRace : public BaseMenu
{
	int height_{ 10 };
	int width_{ 13 };
	int starty_{ (LINES - height_) / 2 };
	int startx_{ (COLS - width_) / 2 };
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
	MenuRace(GameContext& ctx);
	~MenuRace();

	void draw();
	void menu(GameContext& ctx) override;
	void on_key(int key, GameContext& ctx);
};
