// file: Menu.cpp
#include <iostream>
#include <string>

#include "Menu.h"
//#include "MenuGender.h"
#include "../Game.h"
#include "../IMenuOptions.h"

Menu::Menu()
{
	menu_new(menu_height, menu_width, menu_starty, menu_startx);
	imenuOptions.emplace(MenuOptions::NEW_GAME, std::make_unique<NewGame>());
	imenuOptions.emplace(MenuOptions::LOAD_GAME, std::make_unique<LoadGame>());
	imenuOptions.emplace(MenuOptions::OPTIONS, std::make_unique<Options>());
	imenuOptions.emplace(MenuOptions::QUIT, std::make_unique<Quit>());
}

Menu::~Menu()
{
	menu_delete();
}

void BaseMenu::menu_new(int height, int width, int starty, int startx)
{
	// check bound before creating window
	if (height >= LINES || width >= COLS)
	{
		game.log("Menu window size is too big. Height: " + std::to_string(height) + ", Width: " + std::to_string(width));
		game.log("Terminal size - LINES: " + std::to_string(LINES) + ", COLS: " + std::to_string(COLS));
		std::exit(EXIT_FAILURE);
	}

	if (starty < 0 || startx < 0 || starty >= 29 || startx >= 119)
	{
		game.log("Menu window start position is out of bounds. StartY: " + std::to_string(starty) + ", StartX: " + std::to_string(startx));
		std::exit(EXIT_FAILURE);
	}

	// create window (height, width, starty, startx)
	menuWindow = newwin(height, width, starty, startx);
}

void Menu::menu_print_option(MenuOptions option)
{
	const int optionToPrint = static_cast<std::underlying_type_t<MenuOptions>>(option);
	
	if (newOption == optionToPrint)
	{
		menu_highlight_on();
	}

	const auto& menuOptionString = menu_get_string(option);
	menu_print(1, optionToPrint, menuOptionString);

	if (newOption == optionToPrint)
	{
		menu_highlight_off();
	}
}

void Menu::menu()
{
	//MenuGender menuGender; // set the next menu in the chain -> menuGender

	while (run) // menu has its own loop
	{
		menu_clear(); // clear menu window each frame

		// this is for debugging the currentOption number
		mvwprintw(menuWindow, 0, 0, "%d", currentOption);

		// print menu options
		for (size_t i = 1; i <= menuOptions.size(); i++)
		{
			menu_print_option(static_cast<MenuOptions>(i));
		}

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
			/*menu_set_run_false();*/ // stop running this menu loop
			imenuOptions.find(currentOption)->second->on_selection(); // run the selected option

			//switch (currentOption)
			//{

			//case MenuOptions::NEW_GAME:
			//{
			//	menuGender.menu_gender(); // go to next menu

			//	if (menuGender.currentGenderOption == MenuGender::MenuGenderOptions::BACK)
			//	{
			//		// reset flags to default
			//		menu_set_run_true(); // keep menu on
			//		menuGender.run = true; // keep running the next menu
			//		break; // break out of switch and keep running menu loop
			//	}
			//	else // if back was not hit
			//	{
			//		game.init(); // after all the menus are done, start a new game
			//	}

			//	break; // break out of switch and start running game loop
			//}

			//case MenuOptions::LOAD_GAME:
			//{
			//	game.load_all();
			//	break;
			//}

			//case MenuOptions::OPTIONS:
			//{
			//	// do nothing
			//	menu_set_run_true();
			//	break;
			//}

			//case MenuOptions::QUIT:
			//{
			//	game.run = false;
			//	game.shouldSave = false;
			//	std::cout << "You quit without saving!" << std::endl;
			//	break;
			//}

			//default:throw std::runtime_error("menu() - switch (currentOption) case key_enter - default: reached!");
			//} // end of switch (currentOption) case key_enter
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
	game.menus.pop_front();
}

// end of file: Menu.cpp
