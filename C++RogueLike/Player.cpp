#include "Player.h"
#include "Actor.h"
#include "globals.h"
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
	attacker = std::make_unique<Attacker>(Attacker(d.d4()));

	destructible = std::make_unique<PlayerDestructible>(PlayerDestructible(30, 2, "dead player", 100));
	ai = std::make_unique<PlayerAi>();
}

bool Player::player_is_dead()
{
	return true;
}

void Player::update()
{
	ai->update(*this);
}

void Player::draw()
{
	attron(COLOR_PAIR(col));
	mvaddch(posY, posX, ch);
	attroff(COLOR_PAIR(col));
}