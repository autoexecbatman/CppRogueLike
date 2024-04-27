// file: Menu.cpp
#include <iostream>
#include <string>

#include "Menu.h"
#include "MenuGender.h"
#include "Game.h"

void Menu::menu_new(int height, int width, int starty, int startx)
{
	// check bound before creating window
	if (height >= LINES || width >= COLS) {
		game.log("Menu window size is too big. Height: " + std::to_string(height) + ", Width: " + std::to_string(width));
		game.log("Terminal size - LINES: " + std::to_string(LINES) + ", COLS: " + std::to_string(COLS));
		std::exit(EXIT_FAILURE);
	}

	if (starty < 0 || startx < 0 || starty >= 29 || startx >= 119) {
		game.log("Menu window start position is out of bounds. StartY: " + std::to_string(starty) + ", StartX: " + std::to_string(startx));
		std::exit(EXIT_FAILURE);
	}

	// create window (height, width, starty, startx)
	menuWindow = newwin(height, width, starty, startx);
	if (menuWindow == nullptr) {
		game.log("menu_new() - newwin() failed!");
		std::exit(EXIT_FAILURE);
	}
	else {
		game.log("menu_new() - newwin() called!");
	}
}

void Menu::menu_clear()
{
	const int result = wclear(menuWindow);

	if (result == ERR)
	{
		game.log("menu_clear() - wclear(menuWindow) failed!");
		std::exit(EXIT_FAILURE);
	}
	else
	{
		game.log("menu_clear() - wclear(menuWindow) called successfully!");
	}
}


void Menu::menu_print(int x, int y, const std::string& text)
{
	const int result = mvwprintw(menuWindow, y, x, text.c_str());

	if (result == ERR)
	{
		game.log("menu_print() - mvwprintw(menuWindow, y, x, text.c_str()) failed!");
		std::exit(EXIT_FAILURE);
	}
	else
	{
		game.log("menu_print() - mvwprintw(menuWindow, y, x, text.c_str()) called successfully!");
	}
}

void Menu::menu_refresh()
{
	const int result = wrefresh(menuWindow);

	if (result == ERR)
	{
		game.log("menu_refresh() - wrefresh(menuWindow) failed!");
		std::exit(EXIT_FAILURE);
	}
	else
	{
		game.log("menu_refresh() - wrefresh(menuWindow) called successfully!");
	}
}

void Menu::menu_delete()
{
	const int result = delwin(menuWindow);
	if (result == ERR)
	{
		game.log("menu_delete() - delwin(menuWindow) failed!");
		std::exit(EXIT_FAILURE);
	}
	else {
		game.log("menu_delete() - delwin(menuWindow) called successfully!");
	}
}

void Menu::menu_highlight_on()
{
	if (menuWindow == nullptr) {
		game.log("menu_highlight_on() - Attempted to call wattron on a null window!");
		std::exit(EXIT_FAILURE);
	}

	wattron(menuWindow, A_REVERSE);
	game.log("menu_highlight_on() - wattron(menuWindow, A_REVERSE) called!");
}


void Menu::menu_highlight_off()
{
	if (menuWindow == nullptr) {
		game.log("menu_highlight_off() - Attempted to call wattroff on a null window!");
		std::exit(EXIT_FAILURE);
	}

	wattroff(menuWindow, A_REVERSE);
	game.log("menu_highlight_off() - wattroff(menuWindow, A_REVERSE) called!");
}

void Menu::menu_print_option(MenuOptions option, int row)
{
	game.log("menu_print_option() - Start. Option: " + std::to_string(static_cast<std::underlying_type_t<MenuOptions>>(option)) + ", Row: " + std::to_string(row));

	if (newOption == static_cast<std::underlying_type_t<MenuOptions>>(option))
	{
		game.log("menu_print_option() - Applying highlight.");
		menu_highlight_on();
	}

	const auto& menuOptionString = menu_get_string(option);

	game.log("menu_print_option() - Printing option string: " + menuOptionString);
	menu_print(1, row, menuOptionString);

	if (newOption == static_cast<std::underlying_type_t<MenuOptions>>(option))
	{
		game.log("menu_print_option() - Removing highlight.");
		menu_highlight_off();
	}

	game.log("menu_print_option() - End.");
}

void Menu::menu_key_listen() 
{
	game.log("Menu::menu_key_listen() - Waiting for key press");
	keyPress = getch();
	if (keyPress == ERR) {
		game.log("Menu::menu_key_listen() - getch() returned ERR! Exiting function.");
		std::exit(ERR);
	}
}

// this function returns the button name
std::string Menu::menu_get_string(MenuOptions option) noexcept
{
	game.log("menu_get_string() - Start. Option: " + std::to_string(static_cast<std::underlying_type_t<MenuOptions>>(option)));

	switch (option)
	{
	case MenuOptions::NEW_GAME:
		return "New Game";

	case MenuOptions::LOAD_GAME:
		return "Load Game";

	case MenuOptions::OPTIONS:
		return "Options";

	case MenuOptions::QUIT:
		return "Quit";

	default:
		game.log("menu_get_string() - Invalid MenuOption encountered!");
		return "Error";
	}
}

void Menu::menu()
{
	// make the menu window
	menu_new(menu_height, menu_width, menu_starty, menu_startx);

	// set the next menu in the chain -> menuGender
	MenuGender menuGender; 

	while (run) // menu has its own loop
	{
		menu_clear(); // clear menu window each frame

		// this is for debugging the currentOption number
		mvwprintw(menuWindow, 0, 0, "%d", currentOption);

		// print menu options
		menu_print_option(MenuOptions::NEW_GAME, 1);
		menu_print_option(MenuOptions::LOAD_GAME, 2);
		menu_print_option(MenuOptions::OPTIONS, 3);
		menu_print_option(MenuOptions::QUIT, 4);
		/*box(menuWindow, 0, 0);*/
		menu_refresh(); // refresh menu window each frame to show changes

		menu_key_listen(); // listen for key presses
		switch (keyPress)
		{

		case KEY_UP:
		{
			newOption--; // decrement newOption
			currentOption = static_cast<MenuOptions>(newOption); // set currentOption to newOption
			break; // break out of switch keep running menu loop
		}

		case KEY_DOWN:
		{
			newOption++; // increment newOption
			currentOption = static_cast<MenuOptions>(newOption); // set currentOption to newOption
			break; // break out of switch keep running menu loop
		}

		case 'Q':
		case 'q':
		{
			menu_set_run_false(); // stop running menu loop
			game.run = false; // stop running game loop
			game.shouldSave = false; // don't save the game
			game.log("Q/q has been pressed.\n You quit without saving.");
			break; // break out of switch and start closing the game
		}

		case 10:
		{ // if a selection is made
			menu_set_run_false(); // stop running this menu loop
			switch (currentOption)
			{

			case MenuOptions::NEW_GAME:
			{
				menuGender.menu_gender(); // go to next menu

				if (menuGender.currentGenderOption == MenuGender::MenuGenderOptions::BACK)
				{
					// reset flags to default
					menu_set_run_true(); // keep menu on
					menuGender.run = true; // keep running the next menu
					break; // break out of switch and keep running menu loop
				}
				else // if back was not hit
				{
					game.init(); // after all the menus are done, start a new game
				}

				break; // break out of switch and start running game loop
			}

			case MenuOptions::LOAD_GAME:
			{
				game.load_all();
				break;
			}

			case MenuOptions::OPTIONS:
			{
				// do nothing
				menu_set_run_true();
				break;
			}

			case MenuOptions::QUIT:
			{
				game.run = false;
				game.shouldSave = false;
				std::cout << "You quit without saving!" << std::endl;
				break;
			}

			default:throw std::runtime_error("menu() - switch (currentOption) case key_enter - default: reached!");
			} // end of switch (currentOption) case key_enter
			break;
		}

		case 'N':
		case 'n':
		{
			menu_set_run_false();
			game.init();
			break;
		}

		case 'L':
		case 'l':
		{
			menu_set_run_false();
			game.load_all();
			break;
		}

		case 'O':
		case 'o':
		{
			// do nothing
			menu_set_run_true();
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
