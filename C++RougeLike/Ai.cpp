#include <iostream>

#include "main.h"

//====
//a constexpr for tracking the number of turns
constexpr auto TRACKING_TURNS = 3;

//====
MonsterAi::MonsterAi() : moveCount(0) {}

void MonsterAi::update(Actor* owner)
{
	if (owner->destructible && owner->destructible->isDead())
	{
		return;
	}

	if (engine.map->isInFov(owner->x, owner->y))
	{
		moveCount = TRACKING_TURNS;
	}
	else
	{
		moveCount--;
	}

	if (moveCount > 0)
	{
		moveOrAttack(owner, engine.player->x,engine.player->y);
	}
}

void MonsterAi::moveOrAttack(Actor* owner, int targetx, int targety)
{
	int dx = targetx - owner->x;
	int dy = targety - owner->y;
	int stepdx = (dx > 0 ? 1 : -1);
	int stepdy = (dy > 0 ? 1 : -1);
	double distance = sqrt(dx * dx + dy * dy);
	if (distance >= 2)
	{
		dx = (int)(round(dx / distance));
		dy = (int)(round(dy / distance));
		if (engine.map->canWalk(owner->x + dx, owner->y + dy))
		{
			owner->x += dx;
			owner->y += dy;
		}
		else if (engine.map->canWalk(owner->x + stepdx, owner->y))
		{
			owner->x += stepdx;
		}
		else if (engine.map->canWalk(owner->x, owner->y + stepdy))
		{
			owner->y += stepdy;
		}
	}
	else if (owner->attacker)
	{
		owner->attacker->attack(owner, engine.player);
	}
}


//====
// how many turns the monster chases the player
// after losing his sight

void PlayerAi::update(Actor* owner)
{
	if (owner->destructible && owner->destructible->isDead())
	{
		return;
	}
	
	int dx = 0, dy = 0;
	int key = getch();
	clear();
	switch (key) // TODO : Correct ?
	{
	case '8':
		dy = -1;
		break;
	case '2':
		dy = 1;
		break;
	case '4':
		dx = -1;
		break;
	case '6':
		dx = 1;
		break;
	// if numpad key 7 is pressed move diagonaly up left
	case '7':
		dx = -1;
		dy = -1;
		break;
		// if numpad key 9 is pressed move diagonaly up right
	case '9':
		dx = 1;
		dy = -1;
		break;
	// if numpad key 1 is pressed move diagonaly down left
	case '1':
		dx = -1;
		dy = 1;
		break;
	// if numpad key 3 is pressed move diagonaly down right
	case '3':
		dx = 1;
		dy = 1;
		break;
	// numpad key 5 is pressed a turn will pass
	case '5':
		engine.gameStatus = Engine::NEW_TURN;
		break;
	case ' ': 
		engine.player->attacker->attack(engine.player, engine.player);
		break;
	case QUIT:
		exit(0);
	}

	if (dx!=0||dy!=0)
	{
		engine.gameStatus = Engine::NEW_TURN;
		if (moveOrAttack(owner,owner->x+dx,owner->y+dy))
		{
			engine.map->computeFov();
		}
	}
}

bool PlayerAi::moveOrAttack(Actor* owner, int targetx, int targety) 
{
	if (engine.map->isWall(targety, targetx))
	{
		return false;
	}

	// look for living actors to attack
	for (const auto& actor : engine.actors)
	{
		if (
			actor->destructible
			&&
			!actor->destructible->isDead()
			&&
			actor->x == targetx
			&&
			actor->y == targety
			)
		{
			owner->attacker->attack(owner, actor);
			return false;
		}
	}

	// look for corpses
	for (const auto& actor : engine.actors)
	{
		if (
			actor->destructible 
			&& 
			actor->destructible->isDead()
			&& 
			actor->x == targetx
			&&
			actor->y == targety
			)
		{
		//std::cout <<
		//	"There's a %s here\n"
		//	<< std::endl;

			mvprintw(
				29,
				0,
				"There's a %s here\n",
				actor->name
			);
		}
	}

		owner->x = targetx;
		owner->y = targety;

		return true;
}

//====