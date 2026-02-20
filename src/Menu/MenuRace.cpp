// file: MenuRace.cpp
#include "MenuRace.h"
#include "MenuName.h"
#include "MenuClass.h"
#include "MenuGender.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"

void Human::on_selection(GameContext& ctx)
{
	ctx.player->playerRace = "Human";
	ctx.player->playerRaceState = Player::PlayerRaceState::HUMAN;
}

void Dwarf::on_selection(GameContext& ctx)
{
	ctx.player->playerRace = "Dwarf";
	ctx.player->playerRaceState = Player::PlayerRaceState::DWARF;
}

void Elf::on_selection(GameContext& ctx)
{
	ctx.player->playerRace = "Elf";
	ctx.player->playerRaceState = Player::PlayerRaceState::ELF;
}

void Gnome::on_selection(GameContext& ctx)
{
	ctx.player->playerRace = "Gnome";
	ctx.player->playerRaceState = Player::PlayerRaceState::GNOME;
}

void HalfElf::on_selection(GameContext& ctx)
{
	ctx.player->playerRace = "Half-Elf";
	ctx.player->playerRaceState = Player::PlayerRaceState::HALFELF;
}

void Halfling::on_selection(GameContext& ctx)
{
	ctx.player->playerRace = "Halfling";
	ctx.player->playerRaceState = Player::PlayerRaceState::HALFLING;
}

void RaceRandom::on_selection(GameContext& ctx)
{
	switch (ctx.dice->d6())
	{
	case 1:
		ctx.player->playerRace = "Human";
		ctx.player->playerRaceState = Player::PlayerRaceState::HUMAN;
		break;
	case 2:
		ctx.player->playerRace = "Dwarf";
		ctx.player->playerRaceState = Player::PlayerRaceState::DWARF;
		break;
	case 3:
		ctx.player->playerRace = "Elf";
		ctx.player->playerRaceState = Player::PlayerRaceState::ELF;
		break;
	case 4:
		ctx.player->playerRace = "Gnome";
		ctx.player->playerRaceState = Player::PlayerRaceState::GNOME;
		break;
	case 5:
		ctx.player->playerRace = "Half-Elf";
		ctx.player->playerRaceState = Player::PlayerRaceState::HALFELF;
		break;
	case 6:
		ctx.player->playerRace = "Halfling";
		ctx.player->playerRaceState = Player::PlayerRaceState::HALFLING;
		break;
	default:break;
	}
}

void RaceBack::on_selection(GameContext& ctx)
{
	ctx.menus->back()->back = true;
}

MenuRace::MenuRace(GameContext& ctx)
{
	int vcols = ctx.renderer ? ctx.renderer->get_viewport_cols() : 60;
	int vrows = ctx.renderer ? ctx.renderer->get_viewport_rows() : 34;
	starty_ = (vrows - height_) / 2;
	startx_ = (vcols - width_) / 2;
	menu_new(height_, width_, starty_, startx_, ctx);
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
	auto row = static_cast<int>(option) + 1; // Start at row 1 after title
	if (currentState == option)
	{
		menu_highlight_on();
	}
	menu_print(1, row, menu_race_get_string(option));
	if (currentState == option)
	{
		menu_highlight_off();
	}
}

void MenuRace::draw()
{
	menu_clear();
	menu_draw_box();
	menu_draw_title("SELECT RACE", YELLOW_BLACK_PAIR);
	for (size_t i{ 0 }; i < menuRaceStrings.size(); ++i)
		menu_race_print_option(static_cast<MenuRaceOptions>(i));
	menu_refresh();
}

void MenuRace::on_key(int key, GameContext& ctx)
{
	switch (keyPress)
	{

	case 0x103:
	case 'w':
	{
		currentState = static_cast<MenuRaceOptions>((static_cast<size_t>(currentState) + iMenuStates.size() - 1) % iMenuStates.size());
		break;
	}

	case 0x102:
	case 's':
	{
		currentState = static_cast<MenuRaceOptions>((static_cast<size_t>(currentState) + 1) % iMenuStates.size());
		break;
	}

	case 10: // enter
	{
		menu_set_run_false(); // exit current menu loop either way if a selection was made
		iMenuStates.at(currentState)->on_selection(ctx); // call on_selection for the selected menu option
		if (currentState != MenuRaceOptions::BACK)
		{
			ctx.menus->push_back(std::make_unique<MenuClass>(ctx));
		}
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

void MenuRace::menu(GameContext& ctx)
{
	while (run && !WindowShouldClose())
	{
		draw();
		menu_key_listen();
		on_key(keyPress, ctx);
	}
	// TODO: screen clearing handled by Renderer
}

// end of file: MenuRace.cpp
