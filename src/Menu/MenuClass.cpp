// file: MenuClass.cpp
#include <algorithm>

#include "MenuClass.h"
#include "MenuName.h"
#include "../Random/RandomDice.h"
#include "../ActorTypes/Player.h"
#include "../Items/Items.h"
#include "../Items/Armor.h"
#include "../Actor/Pickable.h"
#include "../Actor/InventoryOperations.h"
#include "../ActorTypes/Healer.h"
#include "../Factories/ItemCreator.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"

using namespace InventoryOperations; // For clean function calls

void Fighter::on_selection(GameContext& ctx)
{
	ctx.player->playerClass = "Fighter";
	ctx.player->playerClassState = Player::PlayerClassState::FIGHTER;
	// Starting gear equipped in Game::update() STARTUP phase after init()
}

void Rogue::on_selection(GameContext& ctx)
{
	ctx.player->playerClass = "Rogue";
	ctx.player->playerClassState = Player::PlayerClassState::ROGUE;
}

void Cleric::on_selection(GameContext& ctx)
{
	ctx.player->playerClass = "Cleric";
	ctx.player->playerClassState = Player::PlayerClassState::CLERIC;
}

void Wizard::on_selection(GameContext& ctx)
{
	ctx.player->playerClass = "Wizard";
	ctx.player->playerClassState = Player::PlayerClassState::WIZARD;
}

void ClassRandom::on_selection(GameContext& ctx)
{
	switch (ctx.dice->d4())
	{
	case 1:
		ctx.player->playerClass = "Fighter";
		ctx.player->playerClassState = Player::PlayerClassState::FIGHTER;
		break;
	case 2:
		ctx.player->playerClass = "Rogue";
		ctx.player->playerClassState = Player::PlayerClassState::ROGUE;
		break;
	case 3:
		ctx.player->playerClass = "Wizard";
		ctx.player->playerClassState = Player::PlayerClassState::WIZARD;
		break;
	case 4:
		ctx.player->playerClass = "Cleric";
		ctx.player->playerClassState = Player::PlayerClassState::CLERIC;
		break;
	default:break;
	}
}

void ClassBack::on_selection(GameContext& ctx)
{
	ctx.menus->back()->back = true;
}

MenuClass::MenuClass(GameContext& ctx)
{
	menu_new(menu_height, menu_width, menu_starty, menu_startx, ctx);
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
	auto row = static_cast<int>(option) + 1; // Start at row 1 after title
	if (currentState == option)
	{
		menu_highlight_on();
	}
	menu_print(1, row, menu_class_get_string(option));
	if (currentState == option)
	{
		menu_highlight_off();
	}
}

void MenuClass::draw()
{
	menu_clear();
	// TODO: draw box (was curses box)
	// TODO: draw title "Select Class" (was curses mvwprintw)
	for (size_t i{ 0 }; i < menuClassStrings.size(); ++i)
	{
		menu_class_print_option(static_cast<MenuState>(i));
	}
	menu_refresh();
}

void MenuClass::on_key(int key, GameContext& ctx)
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
		currentState = static_cast<MenuState>((static_cast<size_t>(currentState) + iMenuStates.size() + 1) % iMenuStates.size());
		break;
	}

	case 10: // enter
	{
		menu_set_run_false();
		iMenuStates.at(currentState)->on_selection(ctx);
		if (currentState != MenuState::BACK)
		{
			MenuName menuName;
			menuName.menu_name(ctx);
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

void MenuClass::menu(GameContext& ctx)
{
	while (run)
	{
		draw();
		menu_key_listen();
		on_key(keyPress, ctx);
	}
	// TODO: clear screen when exiting (was curses clear/refresh)
}

// end of file: MenuClass.cpp
