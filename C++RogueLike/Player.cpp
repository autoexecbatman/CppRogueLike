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

	attacker = std::make_shared<Attacker>(dmg, minDmg, maxDmg);
	destructible = std::make_shared<PlayerDestructible>(maxHp, dr, corpseName, xp, thaco, armorClass);
	ai = std::make_shared<AiPlayer>();
	container = std::make_shared<Container>(26);
	this->canSwim = canSwim;
}

bool Player::player_is_dead()
{
	return false;
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
	case Player::PlayerRace::Human:
		break;
	case Player::PlayerRace::Dwarf:
		// write dwarf stuff
		clear();
		mvprintw(0, 0, "You got +1 to constitution and -1 to charisma for being a dwarf.");
		refresh();
		getch();

		game.player->constitution += 1;
		game.player->charisma -= 1;
		break;
	case Player::PlayerRace::Elf:
		// write elf stuff
		clear();
		mvprintw(0, 0, "You got +1 to dexterity and -1 to constitution for being an elf.");
		refresh();
		getch();

		game.player->dexterity += 1;
		game.player->constitution -= 1;
		break;
	case Player::PlayerRace::Gnome:
		// write gnome stuff
		clear();
		mvprintw(0, 0, "You got +1 to intelligence and -1 to wisdom for being a gnome.");
		refresh();
		getch();

		game.player->intelligence += 1;
		game.player->wisdom -= 1;
		break;
	case Player::PlayerRace::HalfElf:
		break;
	case Player::PlayerRace::Halfling:
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

// end of file: Player.cpp
