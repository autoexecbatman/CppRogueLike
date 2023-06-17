// file: MenuClass.cpp
#include "MenuClass.h"
#include "Game.h"
#include "MenuName.h"

std::string MenuClass::menu_class_get_string(MenuClassOptions option) noexcept
{
	switch (option)
	{
	case MenuClassOptions::FIGHTER:
		return "Fighter";
		break;
	case MenuClassOptions::ROGUE:
		return "Rogue";
		break;
	case MenuClassOptions::WIZARD:
		return "Wizard";
		break;
	case MenuClassOptions::CLERIC:
		return "Cleric";
		break;
	case MenuClassOptions::RANDOM:
		return "Random";
		break;
	case MenuClassOptions::BACK:
		return "Back";
		break;
	default:
		return "Error";
		break;
	}
}

void MenuClass::menu_class_print_option(MenuClassOptions option, int row) noexcept
{
	if (newOption == static_cast<std::underlying_type_t<MenuClassOptions>>(option))
	{
		menu_class_highlight_on();
	}
	const auto& menuOptionString = menu_class_get_string(option);
	menu_class_print(1, row, menuOptionString);
	if (newOption == static_cast<std::underlying_type_t<MenuClassOptions>>(option))
	{
		menu_class_highlight_off();
	}
}

void MenuClass::menu_class_select()
{
	switch (currentOption)
	{
	case MenuClassOptions::FIGHTER:
		menu_class_fighter();
		break;
	case MenuClassOptions::ROGUE:
		menu_class_rogue();
		break;
	case MenuClassOptions::WIZARD:
		menu_class_wizard();
		break;
	case MenuClassOptions::CLERIC:
		menu_class_cleric();
		break;
	case MenuClassOptions::RANDOM:
		menu_class_random();
		break;
	case MenuClassOptions::BACK:
		break;
	default:
		break;
	}
}

void MenuClass::menu_class_fighter()
{
	// set player class
	const auto& fighter = menu_class_get_string(MenuClassOptions::FIGHTER);
	game.player->playerClass = fighter;
}

void MenuClass::menu_class_rogue()
{
	// set player class
	const auto& rogue = menu_class_get_string(MenuClassOptions::ROGUE);
	game.player->playerClass = rogue;
}

void MenuClass::menu_class_wizard()
{
	// set player class
	const auto& wizard = menu_class_get_string(MenuClassOptions::WIZARD);
	game.player->playerClass = wizard;
}

void MenuClass::menu_class_cleric()
{
	// set player class
	const auto& cleric = menu_class_get_string(MenuClassOptions::CLERIC);
	game.player->playerClass = cleric;
}

void MenuClass::menu_class_random()
{
	// randomize player class use radom number generator
	const auto& rng = game.random_number(1, 4);
	switch (rng)
	{
		case 1:
		menu_class_fighter();
		break;
		case 2:
		menu_class_rogue();
		break;
		case 3:
		menu_class_wizard();
		break;
		case 4:
		menu_class_cleric();
		break;
		default:break;
	}
}

void MenuClass::menu_class()
{
	menu_class_new(10, 20, (LINES / 2) - 5, (COLS / 2) - 10);
	MenuName menuName;

	while (run)
	{
		menu_class_clear();

		// print menu options
		mvwprintw(
			menuClassWindow,
			0,
			0,
			"Choose a class:");

		menu_class_print_option(MenuClassOptions::FIGHTER, 1);
		menu_class_print_option(MenuClassOptions::ROGUE, 2);
		menu_class_print_option(MenuClassOptions::CLERIC, 3);
		menu_class_print_option(MenuClassOptions::WIZARD, 4);
		menu_class_print_option(MenuClassOptions::RANDOM, 5);
		menu_class_print_option(MenuClassOptions::BACK, 6);

		menu_class_refresh();

		const int input = getch();
		switch (input)
		{

		case KEY_UP:
		{
			menu_class_move_up();
			break;
		}

		case KEY_DOWN:
		{
			menu_class_move_down();
			break;
		}

		case 10: // enter
		{
			run = false; // class loop stop
			menu_class_select();
			if (currentOption == MenuClassOptions::BACK)
			{
				break;
			}
			else
			{
				menuName.menu_name();
			}
			break;
		}

		case 27: // escape
		{
			break;
		}

		default:
			break;
		}

		// check if in bounds of menu options
		if (newOption < 1)
		{
			newOption = 6;
			currentOption = static_cast<MenuClassOptions>(newOption);
		}
		else if (newOption > 6)
		{
			newOption = 1;
			currentOption = static_cast<MenuClassOptions>(newOption);
		}
	}
	menu_class_delete();
}

// end of file: MenuClass.cpp
