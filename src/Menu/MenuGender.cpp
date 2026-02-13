// file: MenuGender.cpp
#include "MenuGender.h"
#include "Menu.h"
#include "MenuClass.h"
#include "MenuRace.h"
#include "../ActorTypes/Player.h"
#include "../Core/GameContext.h"

void Male::on_selection(GameContext& ctx)
{
	ctx.player->set_gender("Male");
}

void Female::on_selection(GameContext& ctx)
{
	ctx.player->set_gender("Female");
}

void Random::on_selection(GameContext& ctx)
{
	auto roll = ctx.dice->d2();
	if (roll == 1)
	{
		ctx.player->set_gender("Male");
	}
	else
	{
		ctx.player->set_gender("Female");
	}
}

void Back::on_selection(GameContext& ctx) 
{
	ctx.menus->back()->back = true;
}

MenuGender::MenuGender(GameContext& ctx)
{
	menu_new(height_, width_, starty_, startx_, ctx);
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
	auto row = static_cast<int>(option) + 1; // Start at row 1 after title
	if (currentState == option)
	{
		menu_highlight_on();
	}
	menu_print(1, row, menu_gender_get_string(option));
	if (currentState == option)
	{
		menu_highlight_off();
	}
}

void MenuGender::draw()
{
	menu_clear();
	// TODO: draw box (was curses box)
	// TODO: draw title "Select Gender" (was curses mvwprintw)
	for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
	{
		menu_print_state(static_cast<MenuState>(i));
	}
	menu_refresh();
}

void MenuGender::on_key(int key, GameContext& ctx)
{
	switch (keyPress)
	{

	case 0x103: // KEY_UP
	case 'w':
	{
		currentState = static_cast<MenuState>((static_cast<size_t>(currentState) + iMenuStates.size() - 1) % iMenuStates.size());
		break;
	}

	case 0x102: // KEY_DOWN
	case 's':
	{
		currentState = static_cast<MenuState>((static_cast<size_t>(currentState) + 1) % iMenuStates.size());
		break;
	}

	case 'M':
	case 'm':
	{
		iMenuStates.at(MenuState::MALE)->on_selection(ctx);
		break;
	}

	case 'F':
	case 'f':
	{
		iMenuStates.at(MenuState::FEMALE)->on_selection(ctx);
		break;
	}

	case 10:
	{
		menu_set_run_false();
		iMenuStates.at(currentState)->on_selection(ctx); // run the selected option
		if (currentState != MenuState::BACK)
		{
			ctx.menus->push_back(std::make_unique<MenuRace>(ctx));
		}
		break;
	}

	default:break;
	} // !end switch keyPress
}

void MenuGender::menu(GameContext& ctx)
{
	while (run) // menu has its own loop
	{
		draw();
		menu_key_listen();
		on_key(keyPress, ctx);
	}
	// TODO: clear screen when exiting (was curses clear/refresh)
}

// end of file: MenuGender.cpp
