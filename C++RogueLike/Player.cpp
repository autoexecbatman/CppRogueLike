// file: Player.cpp
#include <gsl/util>
#include "Game.h"
#include "Map.h"
#include "Ai.h"
#include "Player.h"
#include "Actor.h"
#include "RandomDice.h"
#include "Colors.h"
#include "curses.h"

Player::Player(int y, int x)
	:
	Actor(
		y, // int y
		x, // int x
		'@', // char ch
		"player", // std::string name
		PLAYER_PAIR, // int col
		0 // int index
	)
{
	RandomDice d;
	posY = y;
	posX = x;
	ch = '@';
	name = "player";
	col = PLAYER_PAIR;
	blocks = true;
	fovOnly = true;
	attacker = std::make_shared<Attacker>(Attacker(d.d4()));

	destructible = std::make_shared<PlayerDestructible>(PlayerDestructible(30, 2, "dead player", 100));
	ai = std::make_shared<PlayerAi>();
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