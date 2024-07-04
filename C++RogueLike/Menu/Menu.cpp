// file: Menu.cpp
#include <string>

#include "Menu.h"
#include "MenuGender.h"
#include "../Game.h"

void NewGame::on_selection()
{
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
	auto row = static_cast<std::underlying_type_t<MenuState>>(state);	
	if (stateEnum == state)
	{
		menu_highlight_on();
	}
	menu_print(1, row, menu_get_string(state));
	if (stateEnum == state)
	{
		menu_highlight_off();
	}
}

void Menu::draw()
{
	menu_clear(); // clear menu window each frame
	mvwprintw(menuWindow, 0, 0, "%d", stateEnum); // this is for debugging the currentOption number
	// print the buttons to the menu window
	for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
	{
		menu_print_state(static_cast<MenuState>(i));
	}
	menu_refresh(); // refresh menu window each frame to show changes
}

void Menu::on_key(int key)
{
	switch (key)
	{

	case KEY_UP:
	{
		stateEnum = static_cast<MenuState>((static_cast<size_t>(stateEnum) + iMenuStates.size() - 1) % iMenuStates.size());
		break; // break out of switch keep running menu loop
	}

	case KEY_DOWN:
	{
		stateEnum = static_cast<MenuState>((static_cast<size_t>(stateEnum) + 1) % iMenuStates.size());
		break; // break out of switch keep running menu loop
	}

	case 10:
	{ // if a selection is made
		menu_set_run_false(); // stop running this menu loop
		iMenuStates.at(stateEnum)->on_selection(); // run the selected option
		break;
	}

	case 'N':
	case 'n':
	{
		menu_set_run_false();
		iMenuStates.at(MenuState::NEW_GAME)->on_selection();
		break;
	}

	case 'L':
	case 'l':
	{
		menu_set_run_false();
		iMenuStates.at(MenuState::LOAD_GAME)->on_selection();
		break;
	}

	case 'O':
	case 'o':
	{
		menu_set_run_false();
		iMenuStates.at(MenuState::OPTIONS)->on_selection();
		break;
	}

	case 'Q':
	case 'q':
	{
		menu_set_run_false(); // stop running menu loop
		iMenuStates.at(MenuState::QUIT)->on_selection();
		break; // break out of switch and start closing the game
	}

	default:break;
	}
}

void Menu::menu()
{
	while (run) // menu has its own loop
	{
		draw(); // draw the menu
		menu_key_listen(); // listen for key presses
		on_key(keyPress); // run the key press
	}
	game.menus.pop_back(); // remove this menu from the list
}

// end of file: Menu.cpp
