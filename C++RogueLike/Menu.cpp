// file: Menu.cpp
#include <iostream>
#include <string>

#include "Menu.h"
#include "MenuGender.h"
#include "Game.h"

void Menu::menu_print_option(MenuOptions option, int row) noexcept
{
	if (newOption == static_cast<std::underlying_type_t<MenuOptions>>(option))
	{
		menu_highlight_on();
	}

	const auto& menuOptionString = menu_get_string(option);

	menu_print(1, row, menuOptionString);

	if (newOption == static_cast<std::underlying_type_t<MenuOptions>>(option))
	{
		menu_highlight_off();
	}
}

std::string Menu::menu_get_string(MenuOptions option) noexcept
{
	switch (option)
	{

	case MenuOptions::NEW_GAME:
	{
		std::string newGame = "New Game";
		return newGame;	
	}

	case MenuOptions::LOAD_GAME:
	{
		std::string loadGame = "Load Game";
		return loadGame;
	}

	case MenuOptions::OPTIONS:
	{
		std::string options = "Options";
		return options;
	}

	case MenuOptions::QUIT:
	{
		std::string quit = "Quit";
		return quit;
	}

	default:	
	{
		std::string error = "Error";
		return error;
	}

	}
}

void Menu::menu()
{
	menu_new(10, 20, (LINES / 2) - 5, (COLS / 2) - 10);
	box(menuWindow, 0, 0);
	MenuGender menuGender;

	while (run)
	{
		menu_clear();

		// print menu options
		mvwprintw(
			menuWindow,
			0,
			0,
			"%d",
			currentOption
		);

		menu_print_option(MenuOptions::NEW_GAME, 1);
		menu_print_option(MenuOptions::LOAD_GAME, 2);
		menu_print_option(MenuOptions::OPTIONS, 3);
		menu_print_option(MenuOptions::QUIT, 4);
		
		menu_refresh(); 
		
		/*const int input = getch();*/
		/*int input = wgetch(menuWindow);*/
		key_listen();
		switch (keyPress)
		{
		case KEY_UP:
		{
			newOption--;
			currentOption = static_cast<MenuOptions>(newOption);
			break;
		}
		case KEY_DOWN:
		{
			newOption++;
			currentOption = static_cast<MenuOptions>(newOption);
			break;
		}
		case 'Q':
		case 'q':
		{
			menu_set_run_false();
			game.run = false;
			std::cout << "`Q/q` was pressed...You quit without saving!" << std::endl;
			break;
		}
		case 10:
		{
			run = false;
			switch (currentOption)
			{
				case MenuOptions::NEW_GAME:
				{
					// gender selection screen
					menuGender.menu_gender();

					if (menuGender.back)
					{
						run = true;
						menuGender.back = false;
						break;
					}
					else
					{
						game.init();
					}

					break;
				}
				case MenuOptions::LOAD_GAME:
				{
					game.load_all();
					break;
				}
				case MenuOptions::OPTIONS:
				{
					// do nothing
					run = true;
					break;
				}
				case MenuOptions::QUIT:
				{
					game.run = false;
					std::cout << "You quit without saving!" << std::endl;
					break;
				}
			}
			break;
		}
		case 'N':
		case 'n':
			run = false;
			game.init();
			break;
		case 'L':
		case 'l':
		{
			run = false;
			game.load_all();
			break;
		}
		case 'O':
		case 'o':
		{
			// do nothing
			run = true;
			break;
		}

		default:break;
		}

		if (newOption < 1)
		{
			newOption = 4;
			currentOption = static_cast<MenuOptions>(newOption);
		}
		else if (newOption > 4)
		{
			newOption = 1;
			currentOption = static_cast<MenuOptions>(newOption);
		}

	}
	menu_delete();
}

// end of file: Menu.cpp
