// file: MenuRace.cpp
#include "MenuRace.h"
#include "MenuName.h"
#include "MenuClass.h"
#include "MenuGender.h"
#include "../Game.h"

void Human::on_selection()
{
	game.player->playerRace = "Human";
	game.player->playerRaceState = Player::PlayerRaceState::HUMAN;
}

void Dwarf::on_selection()
{
	game.player->playerRace = "Dwarf";
	game.player->playerRaceState = Player::PlayerRaceState::DWARF;
}

void Elf::on_selection()
{
	game.player->playerRace = "Elf";
	game.player->playerRaceState = Player::PlayerRaceState::ELF;
}

void Gnome::on_selection()
{
	game.player->playerRace = "Gnome";
	game.player->playerRaceState = Player::PlayerRaceState::GNOME;
}

void HalfElf::on_selection()
{
	game.player->playerRace = "Half-Elf";
	game.player->playerRaceState = Player::PlayerRaceState::HALFELF;
}

void Halfling::on_selection()
{
	game.player->playerRace = "Halfling";
	game.player->playerRaceState = Player::PlayerRaceState::HALFLING;
}

void RaceRandom::on_selection()
{
	RandomDice d;
	const int rng = d.d6();
	switch (rng)
	{
	case 1:
		game.player->playerRace = "Human";
		game.player->playerRaceState = Player::PlayerRaceState::HUMAN;
		break;
	case 2:
		game.player->playerRace = "Dwarf";
		game.player->playerRaceState = Player::PlayerRaceState::DWARF;
		break;
	case 3:
		game.player->playerRace = "Elf";
		game.player->playerRaceState = Player::PlayerRaceState::ELF;
		break;
	case 4:
		game.player->playerRace = "Gnome";
		game.player->playerRaceState = Player::PlayerRaceState::GNOME;
		break;
	case 5:
		game.player->playerRace = "Half-Elf";
		game.player->playerRaceState = Player::PlayerRaceState::HALFELF;
		break;
	case 6:
		game.player->playerRace = "Halfling";
		game.player->playerRaceState = Player::PlayerRaceState::HALFLING;
		break;
	default:break;
	}
}

void RaceBack::on_selection()
{
}

MenuRace::MenuRace()
{
	menu_new(height_, width_, starty_, startx_);
	iMenuStates.emplace(MenuRaceOptions::HUMAN, std::make_unique<Human>());
	iMenuStates.emplace(MenuRaceOptions::DWARF, std::make_unique<Dwarf>());
	iMenuStates.emplace(MenuRaceOptions::ELF, std::make_unique<Elf>());
	iMenuStates.emplace(MenuRaceOptions::GNOME, std::make_unique<Gnome>());
	iMenuStates.emplace(MenuRaceOptions::HALFELF, std::make_unique<HalfElf>());
	iMenuStates.emplace(MenuRaceOptions::HALFLING, std::make_unique<Halfling>());
	iMenuStates.emplace(MenuRaceOptions::RANDOM, std::make_unique<Random>());
	iMenuStates.emplace(MenuRaceOptions::BACK, std::make_unique<Back>());
}

MenuRace::~MenuRace()
{
	menu_delete();
}

void MenuRace::menu_race_print_option(MenuRaceOptions option) noexcept
{
	auto row = static_cast<int>(option);
	if (stateEnum == option)
	{
		menu_highlight_on();
	}
	menu_print(1, row, menu_race_get_string(option));
	if (stateEnum == option)
	{
		menu_highlight_off();
	}
}

void MenuRace::menu()
{
	MenuClass menuClass;
	while (run)
	{
		menu_clear();
		mvwprintw(menuWindow, 0, 0, "Choose a race:");

		for (size_t i{ 0 }; i < menuRaceStrings.size(); ++i)
		{
			menu_race_print_option(static_cast<MenuRaceOptions>(i));
		}
		menu_refresh();

		menu_key_listen();
		switch (keyPress)
		{

		case KEY_UP:
		{
			stateInt = (stateInt - 1) % iMenuStates.size();
			stateEnum = static_cast<MenuRaceOptions>(stateInt);
			break;
		}

		case KEY_DOWN:
		{
			stateInt = (stateInt + 1) % iMenuStates.size();
			stateEnum = static_cast<MenuRaceOptions>(stateInt);
			break;
		}

		case 10: // enter
		{
			menu_set_run_false(); // exit current menu loop either way if a selection was made
			iMenuStates.at(stateEnum)->on_selection(); // call on_selection for the selected menu option
			break; // break and go back to previous menu (menuGender)
		}

		case 27: // escape
		{
			break;
		}
		default:
			break;
		} // end switch (input)
	}
}

// end of file: MenuRace.cpp
