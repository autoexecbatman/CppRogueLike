// file: MenuGender.cpp
#include "MenuGender.h"
#include "Menu.h"
#include "MenuClass.h"
#include "MenuRace.h"
#include "../Game.h"
#include "../ActorTypes/Player.h"

void MenuGender::menu_gender_clear() noexcept
{
	wclear(menuGenderWindow);
	game.log("MenuGender cleared successfully.");
}

void MenuGender::menu_gender_store(MenuGenderOptions option)
{
	const auto& newGender = menu_gender_get_string(option);
	gender = newGender;
}

void MenuGender::menu_gender_assign()
{
	if (!game.player)
	{
		game.log("void MenuGender::menu_gender_assign() player is nullptr");
		exit(EXIT_FAILURE);
	}
	game.player->gender = gender;
}

void MenuGender::menu_gender_assign_random()
{
	const Menu menu;
	// roll a random number between 1 and 2
	RandomDice d;
	const int rng = d.d2();

	// if the number is 1 return "Male"
	switch(rng)
	{
	case 1:
	{
		// game.player->gender = ?
		const auto& male = menu_gender_get_string(MenuGenderOptions::MALE);
		game.player->gender = male;
	}
	break;
	case 2:
	{
		const auto& female = menu_gender_get_string(MenuGenderOptions::FEMALE);
		game.player->gender = female;
	}
	break;
	default:
		break;
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
		break;
	}

	default:break;
	} // !end switch currentGenderOption 
}

void MenuGender::menu_gender()
{
	// make the gender selection menu window
	menu_gender_new(height_,width_,starty_,startx_);

	// set the next menu in the chain -> menuRace
	MenuRace menuRace;

	while (run) // menu has its own loop
	{
		menu_gender_clear(); // clear the window

		// print the menu options to the top of the window
		mvwprintw(menuGenderWindow, 0, 0, "%d", currentGenderOption);

		menu_gender_print_option(MenuGenderOptions::MALE, 1);
		menu_gender_print_option(MenuGenderOptions::FEMALE, 2);
		menu_gender_print_option(MenuGenderOptions::RANDOM, 3);
		menu_gender_print_option(MenuGenderOptions::BACK, 4);

		menu_gender_refresh();

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
			menu_gender_set_run_false(); // loop is terminated either way
			menu_gender_select(); // switch selection
			if (currentGenderOption == MenuGenderOptions::BACK) // if this menu selected back
			{
				break;
			}
			else
			{
				menuRace.menu_race(); // if back was NOT pressed, go to the next menu (menuRace)
				if (menuRace.currentOption == MenuRace::MenuRaceOptions::BACK) // if the next menu selected back
				{
					// set the current menu to run again
					run = true;
					menuRace.run = true;
					break;
				}
				else
				{
					// set the current menu to NOT run again
					run = false;
					menuRace.run = false;
					break;
				}
			}
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
