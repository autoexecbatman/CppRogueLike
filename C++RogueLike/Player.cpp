// file: Player.cpp
#include <gsl/util>
#include "Game.h"
#include "Map.h"
#include "Ai.h"
#include "AiPlayer.h"
#include "Player.h"
#include "Actor.h"
#include "RandomDice.h"
#include "Colors.h"
#include "curses.h"
#include "CalculatedTHAC0s.h"

//Player::Player(int y, int x)
Player::Player(int y, int x, int maxHp, int dr, std::string corpseName, int xp, int thaco, int armorClass, int dmg, int minDmg, int maxDmg, bool canSwim)
	:
	Actor(y, x, '@', "player", PLAYER_PAIR, 0)
{
	RandomDice d;

	blocks = true;
	fovOnly = true;

	strength = d.d6() + d.d6() + d.d6();
	dexterity = d.d6() + d.d6() + d.d6();
	constitution = d.d6() + d.d6() + d.d6();
	intelligence = d.d6() + d.d6() + d.d6();
	wisdom = d.d6() + d.d6() + d.d6();
	charisma = d.d6() + d.d6() + d.d6();

	attacker = std::make_unique<Attacker>(dmg, minDmg, maxDmg);
	destructible = std::make_unique<PlayerDestructible>(maxHp, dr, corpseName, xp, thaco, armorClass);
	ai = std::make_unique<AiPlayer>();
	container = std::make_unique<Container>(26);
	this->canSwim = canSwim;
}

// a function to get postion from the map class using the getter functions
void Player::player_get_pos_from_map()
{
	if (game.map == nullptr)
	{
		std::clog << "game.map is nullptr" << std::endl;
	}
	else
	{
		const auto& map = game.map;
		posX = map->get_player_pos_x();
		posY = map->get_player_pos_y();
	}
}

void Player::racial_ability_adjustments()
{
	// switch player state
	switch (game.player->playerRaceState)
	{
	case Player::PlayerRaceState::HUMAN:
		break;
	case Player::PlayerRaceState::DWARF:
		// write dwarf stuff
		clear();
		mvprintw(0, 0, "You got +1 to constitution and -1 to charisma for being a dwarf.");
		refresh();
		getch();

		game.player->constitution += 1;
		game.player->charisma -= 1;
		break;
	case Player::PlayerRaceState::ELF:
		// write elf stuff
		clear();
		mvprintw(0, 0, "You got +1 to dexterity and -1 to constitution for being an elf.");
		refresh();
		getch();

		game.player->dexterity += 1;
		game.player->constitution -= 1;
		break;
	case Player::PlayerRaceState::GNOME:
		// write gnome stuff
		clear();
		mvprintw(0, 0, "You got +1 to intelligence and -1 to wisdom for being a gnome.");
		refresh();
		getch();

		game.player->intelligence += 1;
		game.player->wisdom -= 1;
		break;
	case Player::PlayerRaceState::HALFELF:
		break;
	case Player::PlayerRaceState::HALFLING:
		// write halfling stuff
		clear();
		mvprintw(0, 0, "You got +1 to dexterity and -1 to strength for being a halfling.");
		refresh();
		getch();

		game.player->dexterity += 1;
		game.player->strength -= 1;
		break;
	default:
		break;
	}
}

void Player::calculate_thaco()
{
	game.err("Calculating THAC0...");
	// print playerClassState

	
	CalculatedTHAC0s thaco;
	switch (playerClassState)
	{	
	case Player::PlayerClassState::FIGHTER:
		game.player->destructible->thaco = thaco.getFighter(playerLevel);
		game.err(playerClass + "you got THAC0: " + std::to_string(game.player->destructible->thaco));
		break;
	case Player::PlayerClassState::ROGUE:
		game.player->destructible->thaco = thaco.getRogue(playerLevel);
		game.err(playerClass + "you got THAC0: " + std::to_string(game.player->destructible->thaco));
		break;
	case Player::PlayerClassState::CLERIC:
		game.player->destructible->thaco = thaco.getCleric(playerLevel);
		game.err(playerClass + "you got THAC0: " + std::to_string(game.player->destructible->thaco));
		break;
	case Player::PlayerClassState::WIZARD:
		game.player->destructible->thaco = thaco.getWizard(playerLevel);
		game.err(playerClass + "you got THAC0: " + std::to_string(game.player->destructible->thaco));
		break;
	default:
		break;
	}
}

// end of file: Player.cpp
