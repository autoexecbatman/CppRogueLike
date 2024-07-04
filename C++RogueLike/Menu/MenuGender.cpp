// file: MenuGender.cpp
#include "MenuGender.h"
#include "Menu.h"
#include "MenuClass.h"
#include "MenuRace.h"
#include "../Game.h"
#include "../ActorTypes/Player.h"

void Male::on_selection()
{
	game.player->gender = "Male";
	game.menus.push_back(std::make_unique<MenuClass>());
}

void Female::on_selection()
{
	game.player->gender = "Female";
}

void Random::on_selection()
{
	auto roll = game.d.d2();
	if (roll == 1)
	{
		game.player->gender = "Male";
	}
	else
	{
		game.player->gender = "Female";
	}
}

void Back::on_selection() 
{
	game.menus.back()->back = true;
}

MenuGender::MenuGender()
{
	menu_new(height_, width_, starty_, startx_);
	iMenuStates.emplace(MenuState::MALE, std::make_unique<Male>());
	iMenuStates.emplace(MenuState::FEMALE, std::make_unique<Female>());
	iMenuStates.emplace(MenuState::RANDOM, std::make_unique<Random>());
	iMenuStates.emplace(MenuState::BACK, std::make_unique<Back>());
}

MenuGender::~MenuGender()
{
	menu_delete();
}

void MenuGender::menu_print_state(MenuState option)
{
	auto row = static_cast<int>(option);
	if (stateEnum == option)
	{
		menu_highlight_on();
	}
	menu_print(1, row, menu_gender_get_string(option));
	if (stateEnum == option)
	{
		menu_highlight_off();
	}
}

void MenuGender::draw()
{
	menu_clear(); // clear the window
	mvwprintw(menuWindow, 0, 0, "%d", stateEnum); // print the menu options to the top of the window
	for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
	{
		menu_print_state(static_cast<MenuState>(i));
	}
	menu_refresh();
}

void MenuGender::on_key(int key)
{
	switch (keyPress)
	{

	case KEY_UP:
	{
		stateEnum = static_cast<MenuState>((static_cast<size_t>(stateEnum) + iMenuStates.size() - 1) % iMenuStates.size());
		break;
	}

	case KEY_DOWN:
	{
		stateEnum = static_cast<MenuState>((static_cast<size_t>(stateEnum) + 1) % iMenuStates.size());
		break;
	}

	case 'M':
	case 'm':
	{
		iMenuStates.at(MenuState::MALE)->on_selection();
		break;
	}

	case 'F':
	case 'f':
	{
		iMenuStates.at(MenuState::FEMALE)->on_selection();
		break;
	}

	case 10:
	{
		menu_set_run_false();
		iMenuStates.at(stateEnum)->on_selection(); // run the selected option
		break;
	}

	default:break;
	} // !end switch keyPress
}

void MenuGender::menu()
{
	while (run) // menu has its own loop
	{
		draw();
		menu_key_listen();
		on_key(keyPress);
	}
}

// end of file: MenuGender.cpp
