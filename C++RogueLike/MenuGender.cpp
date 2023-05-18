// file: MenuGender.cpp
#include "MenuGender.h"
#include "Game.h"
#include "Menu.h"
#include "MenuClass.h"
#include "Player.h"

void MenuGender::menu_gender_store(MenuGenderOptions option)
{
	const auto& newGender = menu_gender_get_string(option);
	gender = newGender;
}

void MenuGender::menu_gender_assign()
{
	/*game.player->gender = gender;*/
	/*Player::gender = gender;*/
}

void MenuGender::menu_gender_assign_random()
{
	const Menu menu;
	// roll a random number between 0 and 1
	const int rng = game.random_number(0, 1);

	// if the number is 0 return "Male"

	if (rng == 0)
	{
		// game.player->gender = ?
		const auto& male = menu_gender_get_string(MenuGenderOptions::MALE);
		game.player->gender = male;
	}
	else
	{
		const auto& female = menu_gender_get_string(MenuGenderOptions::FEMALE);
		game.player->gender = female;
	}
}

void MenuGender::menu_gender_print_option(MenuGenderOptions option, int row) noexcept
{
	if (newOption == static_cast<std::underlying_type_t<MenuGenderOptions>>(option))
	{
		menu_gender_highlight_on();
	}

	const auto& menuOptionString = menu_gender_get_string(option);

	menu_gender_print(1, row, menuOptionString);

	if (newOption == static_cast<std::underlying_type_t<MenuGenderOptions>>(option))
	{
		menu_gender_highlight_off();
	}
}

std::string MenuGender::menu_gender_get_string(MenuGenderOptions option) noexcept
{
	switch (option)
	{

	case MenuGender::MenuGenderOptions::NONE:
	{
		return "None";
		break;
	}

	case MenuGender::MenuGenderOptions::MALE:
	{
		return "Male";
		break;
	}

	case MenuGender::MenuGenderOptions::FEMALE:
	{
		return "Female";
		break;
	}

	case MenuGender::MenuGenderOptions::RANDOM:
	{
		return "Random";
		break;
	}

	case MenuGender::MenuGenderOptions::BACK:
	{
		return "Back";
		break;
	}

	default:
	{
		return "Error";
		break;
	}

	} // !end switch option
}

void MenuGender::menu_gender_select()
{
	switch (currentGenderOption)
	{
	case MenuGenderOptions::MALE:
	{
		menu_gender_store(MenuGenderOptions::MALE);
		menu_gender_assign();

		break;
	}

	case MenuGenderOptions::FEMALE:
	{
		menu_gender_store(MenuGenderOptions::FEMALE);
		menu_gender_assign();
		break;
	}

	case MenuGenderOptions::RANDOM:
	{
		menu_gender_assign_random();
		break;
	}

	case MenuGenderOptions::BACK:
	{
		menu_gender_set_run_false();
		menu_gender_set_back_true();
		break;
	}

	default:break;
	} // !end switch currentGenderOption 
}

void MenuGender::menu_gender()
{
	menu_gender_new(10, 20, (LINES / 2) - 5, (COLS / 2) - 10);
	MenuClass menuClass;

	run = true;
	while (run)
	{
		menu_gender_clear();

		// print the menu options to the top of the window
		mvwprintw(menuGenderWindow, 0, 0, "%d", currentGenderOption);

		menu_gender_print_option(MenuGenderOptions::MALE, 1);
		menu_gender_print_option(MenuGenderOptions::FEMALE, 2);
		menu_gender_print_option(MenuGenderOptions::RANDOM, 3);
		menu_gender_print_option(MenuGenderOptions::BACK, 4);

		menu_gender_refresh();

		/*const int input = getch();*/
		key_listen();
		switch (keyPress)
		{

		case KEY_UP:
		{
			newOption--;
			currentGenderOption = static_cast<MenuGenderOptions>(newOption);
			break;
		}

		case KEY_DOWN:
		{
			newOption++;
			currentGenderOption = static_cast<MenuGenderOptions>(newOption);
			break;
		}

		case 'Q':
		case 'q':
		{
			menu_gender_set_run_false();
			game.run = false;
			std::cout << "`Q/q` was pressed...You quit without saving!" << std::endl;
			break;
		}

		case 'M':
		case 'm':
		{
			menu_gender_store(MenuGenderOptions::MALE);
			menu_gender_assign();
			break;
		}

		case 10:
		{
			menu_gender_set_run_false();
			menu_gender_select();
			menuClass.menu_class();
			break;
		} // !end case 10

		default:break;
		} // !end switch keyPress

		// check if the new option is out of bounds
		if (newOption < 1)
		{
			newOption = 4;
			currentGenderOption = static_cast<MenuGenderOptions>(newOption);
		}
		else if (newOption > 4)
		{
			newOption = 1;
			currentGenderOption = static_cast<MenuGenderOptions>(newOption);
		}

	} // !end while run
	menu_gender_delete();
}

// end of file: MenuGender.cpp
