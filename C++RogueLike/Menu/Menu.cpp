// file: Menu.cpp
#include <string>

#include "Menu.h"
#include "../Game.h"

void NewGame::on_selection()
{
	// set menu run flag to false to halt the while loop
	game.deadMenus.push_back(std::move(game.menus.front()));
	game.menus.push_back(std::make_unique<MenuGender>());
}

void LoadGame::on_selection()
{
	game.load_all();
}

void Options::on_selection()
{
	// do nothing
}

void Quit::on_selection()
{
	game.run = false;
	game.shouldSave = false;
	game.log("You quit without saving!");
}

Menu::Menu()
{
	menu_new(menu_height, menu_width, menu_starty, menu_startx);
	iMenuStates.emplace(MenuState::NEW_GAME, std::make_unique<NewGame>());
	iMenuStates.emplace(MenuState::LOAD_GAME, std::make_unique<LoadGame>());
	iMenuStates.emplace(MenuState::OPTIONS, std::make_unique<Options>());
	iMenuStates.emplace(MenuState::QUIT, std::make_unique<Quit>());
}

Menu::~Menu()
{
	menu_delete();
}

void Menu::menu_print_state(MenuState state)
{
	const int optionToPrint = static_cast<std::underlying_type_t<MenuState>>(state);
	
	if (stateEnum == state)
	{
		menu_highlight_on();
	}

	menu_print(1, optionToPrint, menu_get_string(state));

	if (stateEnum == state)
	{
		menu_highlight_off();
	}
}

void Menu::menu()
{
	while (run) // menu has its own loop
	{
		menu_clear(); // clear menu window each frame
		mvwprintw(menuWindow, 0, 0, "%d", stateEnum); // this is for debugging the currentOption number
		// print the buttons to the menu window
		for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
		{	
			menu_print_state(static_cast<MenuState>(i));
		}
		menu_refresh(); // refresh menu window each frame to show changes
		menu_key_listen(); // listen for key presses
		switch (keyPress)
		{

		case KEY_UP:
		{
			stateInt = (stateInt - 1) % iMenuStates.size();
			stateEnum = static_cast<MenuState>(stateInt);
			break; // break out of switch keep running menu loop
		}

		case KEY_DOWN:
		{
			stateInt = (stateInt + 1) % iMenuStates.size();
			stateEnum = static_cast<MenuState>(stateInt); // set currentOption to newOption
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
			iMenuStates.find(stateEnum)->second->on_selection(); // run the selected option
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
	}
	game.menus.pop_front();
}

// end of file: Menu.cpp
