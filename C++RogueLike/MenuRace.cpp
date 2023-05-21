// file: MenuRace.cpp
#include "MenuRace.h"
#include "Game.h"
#include "MenuName.h"
#include "MenuClass.h"

std::string MenuRace::menu_race_get_string(MenuRaceOptions option) noexcept
{
	switch (option)
	{
	case MenuRaceOptions::HUMAN:
		return "Human";
		break;
	case MenuRaceOptions::DWARF:
		return "Dwarf";
		break;
	case MenuRaceOptions::ELF:
		return "Elf";
		break;
	case MenuRaceOptions::GNOME:
		return "Gnome";
		break;
	case MenuRaceOptions::HALFELF:
		return "Half-Elf";
		break;
	case MenuRaceOptions::HALFLING:
		return "Halfling";
		break;
	case MenuRaceOptions::RANDOM:
		return "Random";
		break;
	case MenuRaceOptions::BACK:
		return "Back";
		break;
	default:
		return "Error";
		break;
	}
}

void MenuRace::menu_race_print_option(MenuRaceOptions option, int row) noexcept
{
	if (newOption == static_cast<std::underlying_type_t<MenuRaceOptions>>(option))
	{
		menu_race_highlight_on();
	}
	const auto& menuOptionString = menu_race_get_string(option);
	menu_race_print(1, row, menuOptionString);
	if (newOption == static_cast<std::underlying_type_t<MenuRaceOptions>>(option))
	{
		menu_race_highlight_off();
	}
}

void MenuRace::menu_race_select()
{
	switch (currentOption)
	{
	case MenuRaceOptions::HUMAN:
		menu_race_human();
		break;
	case MenuRaceOptions::DWARF:
		menu_race_dwarf();
		break;
	case MenuRaceOptions::ELF:
		menu_race_elf();
		break;
	case MenuRaceOptions::GNOME:
		menu_race_gnome();
		break;
	case MenuRaceOptions::RANDOM:
		menu_race_random();
		break;
	case MenuRaceOptions::BACK:
		menu_race_back();
		break;
	default:
		break;
	}
}

void MenuRace::menu_race_human()
{
	// set player race
	const auto& human = menu_race_get_string(MenuRaceOptions::HUMAN);
	game.player->playerRace = human;
}

void MenuRace::menu_race_dwarf()
{
	// set player race
	const auto& dwarf = menu_race_get_string(MenuRaceOptions::DWARF);
	game.player->playerRace = dwarf;
}

void MenuRace::menu_race_elf()
{
	// set player race
	const auto& elf = menu_race_get_string(MenuRaceOptions::ELF);
	game.player->playerRace = elf;
}

void MenuRace::menu_race_gnome()
{
	// set player race
	const auto& gnome = menu_race_get_string(MenuRaceOptions::GNOME);
	game.player->playerRace = gnome;
}

void MenuRace::menu_race_halfelf()
{
	// set player race
	const auto& halfelf = menu_race_get_string(MenuRaceOptions::HALFELF);
	game.player->playerRace = halfelf;
}

void MenuRace::menu_race_halfling()
{
	// set player race
	const auto& halfling = menu_race_get_string(MenuRaceOptions::HALFLING);
	game.player->playerRace = halfling;
}

void MenuRace::menu_race_random()
{
	// randomize player class use radom number generator
	const auto& rng = game.random_number(1, 6);
	switch (rng)
	{
	case 1:
		menu_race_human();
		break;
	case 2:
		menu_race_dwarf();
		break;
	case 3:
		menu_race_elf();
		break;
	case 4:
		menu_race_gnome();
		break;
	case 5:
		menu_race_halfelf();
		break;
	case 6:
		menu_race_halfling();
		break;
	default:break;
	}
}

void MenuRace::menu_race_back()
{
	// go back to previous menu (gender menu)
}

void MenuRace::menu_race()
{
	menu_race_new(10, 20, (LINES / 2) - 5, (COLS / 2) - 10);
	MenuClass menuClass;

	while (run)
	{
		menu_race_clear();

		// print menu options
		mvwprintw(
			menuRaceWindow,
			0,
			0,
			"Choose a race:");

		menu_race_print_option(MenuRaceOptions::HUMAN, 1);
		menu_race_print_option(MenuRaceOptions::DWARF, 2);
		menu_race_print_option(MenuRaceOptions::ELF, 3);
		menu_race_print_option(MenuRaceOptions::GNOME, 4);
		menu_race_print_option(MenuRaceOptions::HALFELF, 5);
		menu_race_print_option(MenuRaceOptions::HALFLING, 6);
		menu_race_print_option(MenuRaceOptions::RANDOM, 7);
		menu_race_print_option(MenuRaceOptions::BACK, 8);

		menu_race_refresh();

		const int input = getch();
		switch (input)
		{

		case KEY_UP:
		{
			menu_race_move_up();
			break;
		}

		case KEY_DOWN:
		{
			menu_race_move_down();
			break;
		}

		case 10: // enter
		{
			run = false;
			menu_race_select();
			menuClass.menu_class();
			break;
		}
		case 27: // escape
			menu_race_back();
			break;
		default:
			break;
		}

		// check if in bounds of menu options
		if (newOption < 1)
		{
			newOption = 8;
			currentOption = static_cast<MenuRaceOptions>(newOption);
		}
		else if (newOption > 8)
		{
			newOption = 1;
			currentOption = static_cast<MenuRaceOptions>(newOption);
		}
	}
	menu_race_delete();
}

// end of file: MenuRace.cpp
