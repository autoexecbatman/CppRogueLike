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
Player::Player(int y, int x, int maxHp, int dr, std::string corpseName, int xp, int dmg, bool canSwim)
	:
	Actor(y, x, '@', "player", PLAYER_PAIR, 0)
{
	RandomDice d;

	blocks = true;
	fovOnly = true;

	strength = 5 + d.d8();

	attacker = std::make_shared<Attacker>(dmg);
	destructible = std::make_shared<PlayerDestructible>(maxHp, dr, corpseName, xp);
	ai = std::make_shared<AiPlayer>();
	container = std::make_shared<Container>(26);
	this->canSwim = canSwim;
}

bool Player::player_is_dead()
{
	return true;
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

// end of file: Player.cpp
