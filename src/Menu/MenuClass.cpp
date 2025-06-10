// file: MenuClass.cpp
#include "MenuClass.h"
#include "MenuName.h"
#include "../Game.h"
#include "../Random/RandomDice.h"
#include "../ActorTypes/Player.h"
#include "../Items.h"
#include "../Armor.h"
#include "../ActorTypes/Healer.h"

void equip_fighter_starting_gear() {
	auto& player = *game.player;
	
	// PLATE MAIL (AC 3) - Auto-equipped
	auto plateMail = std::make_unique<Item>(player.position, ActorData{'[', "plate mail", DOOR_PAIR});
	plateMail->pickable = std::make_unique<PlateMail>();
	plateMail->add_state(ActorState::IS_EQUIPPED);
	player.container->add(std::move(plateMail));
	
	// LONG SWORD (1d8) - Auto-equipped
	auto longSword = std::make_unique<Item>(player.position, ActorData{'/', "long sword", WHITE_PAIR});
	longSword->pickable = std::make_unique<LongSword>();
	longSword->add_state(ActorState::IS_EQUIPPED);
	player.container->add(std::move(longSword));
	
	// Update fighter combat stats
	player.weaponEquipped = "Long Sword";
	player.attacker = std::make_unique<Attacker>("D8");
	player.destructible->armorClass = 3; // Plate mail AC
	player.gold = 200; // Increased starting gold
	
	// HEALING POTIONS (3x)
	for(int i = 0; i < 3; i++) {
		auto healthPotion = std::make_unique<Item>(player.position, ActorData{'!', "health potion", HPBARMISSING_PAIR});
		healthPotion->pickable = std::make_unique<Healer>(10);
		player.container->add(std::move(healthPotion));
	}
}

void Fighter::on_selection()
{
	game.player->playerClass = "Fighter";
	game.player->playerClassState = Player::PlayerClassState::FIGHTER;
	
	// EQUIP FIGHTER STARTING GEAR
	equip_fighter_starting_gear();
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
		equip_fighter_starting_gear();
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
	mvwprintw(menuWindow, 0, 0, "%d", currentState);
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
		currentState = static_cast<MenuState>((static_cast<size_t>(currentState) + iMenuStates.size() - 1) % iMenuStates.size());
		break;
	}

	case KEY_DOWN:
	{
		currentState = static_cast<MenuState>((static_cast<size_t>(currentState) + iMenuStates.size() + 1) % iMenuStates.size());
		break;
	}

	case 10: // enter
	{
		menu_set_run_false();
		iMenuStates.at(currentState)->on_selection();
		if (currentState != MenuState::BACK)
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
