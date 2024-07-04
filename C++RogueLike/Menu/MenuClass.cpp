// file: MenuClass.cpp
#include "MenuClass.h"
#include "MenuName.h"
#include "../Game.h"
#include "../Random/RandomDice.h"
#include "../ActorTypes/Player.h"

void Fighter::on_selection()
{
	game.player->playerClass = "Fighter";
	game.player->playerClassState = Player::PlayerClassState::FIGHTER;
}

void Rogue::on_selection()
{
	game.player->playerClass = "Rogue";
	game.player->playerClassState = Player::PlayerClassState::ROGUE;
}

void Cleric::on_selection()
{
	game.player->playerClass = "Cleric";
	game.player->playerClassState = Player::PlayerClassState::CLERIC;
}

void Wizard::on_selection()
{
	game.player->playerClass = "Wizard";
	game.player->playerClassState = Player::PlayerClassState::WIZARD;
}

void ClassRandom::on_selection()
{
	RandomDice d;
	const int rng = d.d4();
	switch (rng)
	{
	case 1:
		game.player->playerClass = "Fighter";
		game.player->playerClassState = Player::PlayerClassState::FIGHTER;
		break;
	case 2:
		game.player->playerClass = "Rogue";
		game.player->playerClassState = Player::PlayerClassState::ROGUE;
		break;
	case 3:
		game.player->playerClass = "Wizard";
		game.player->playerClassState = Player::PlayerClassState::WIZARD;
		break;
	case 4:
		game.player->playerClass = "Cleric";
		game.player->playerClassState = Player::PlayerClassState::CLERIC;
		break;
	default:break;
	}
}

void ClassBack::on_selection()
{
	game.menus.back()->back = true;
}

MenuClass::MenuClass()
{
	menu_new(menu_height, menu_width, menu_starty, menu_startx);
	iMenuStates.emplace(MenuState::FIGHTER, std::make_unique<Fighter>());
	iMenuStates.emplace(MenuState::ROGUE, std::make_unique<Rogue>());
	iMenuStates.emplace(MenuState::CLERIC, std::make_unique<Cleric>());
	iMenuStates.emplace(MenuState::WIZARD, std::make_unique<Wizard>());
	iMenuStates.emplace(MenuState::RANDOM, std::make_unique<ClassRandom>());
	iMenuStates.emplace(MenuState::BACK, std::make_unique<ClassBack>());
}

MenuClass::~MenuClass()
{
	menu_delete();
}

void MenuClass::menu_class_print_option(MenuState option) noexcept
{
	auto row = static_cast<int>(option);
	if (stateEnum == option)
	{
		menu_highlight_on();
	}
	menu_print(1, row, menu_class_get_string(option));
	if (stateEnum == option)
	{
		menu_highlight_off();
	}
}

void MenuClass::draw()
{
	menu_clear();
	mvwprintw(menuWindow, 0, 0, "%d", stateEnum);
	for (size_t i{ 0 }; i < menuClassStrings.size(); ++i)
	{
		menu_class_print_option(static_cast<MenuState>(i));
	}
	menu_refresh();
}

void MenuClass::on_key(int key)
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
		stateEnum = static_cast<MenuState>((static_cast<size_t>(stateEnum) + iMenuStates.size() + 1) % iMenuStates.size());
		break;
	}

	case 10: // enter
	{
		menu_set_run_false();
		iMenuStates.at(stateEnum)->on_selection();
		if (stateEnum == MenuState::BACK)
		{
			break;
		}
		else
		{
			MenuName menuName;
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
}

void MenuClass::menu()
{
	while (run)
	{
		draw();
		menu_key_listen();
		on_key(keyPress);
	}
}

// end of file: MenuClass.cpp
