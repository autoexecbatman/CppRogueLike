#include "AiMonster.h"
#include "../Game.h"

//==MONSTER_AI==

constexpr auto TRACKING_TURNS = 3; // Used in AiMonster::update()

AiMonster::AiMonster() : moveCount(0) {}

void AiMonster::update(Actor& owner)
{
	if (owner.ai == nullptr) // if the owner has no ai
	{
		return; // do nothing
	}

	if (owner.destructible != nullptr)
	{
		if (owner.destructible->is_dead()) // if the owner is dead
		{
			return; // do nothing
		}
	}
	else
	{
		std::cout << "Error: AiMonster::update() - owner.destructible is null" << std::endl;
		exit(-1);
	}

	if (game.map->is_in_fov(owner.posX, owner.posY)) // if the owner is in the fov
	{
		// move towards the player
		moveCount = TRACKING_TURNS;
	}
	else
	{
		moveCount--; // decrement the move count
	}

	if (moveCount > 0) // if the move count is greater than 0
	{
		moveOrAttack(owner, game.player->posX, game.player->posY); // move or attack the player
	}
}

void AiMonster::load(TCODZip& zip)
{
	moveCount = zip.getInt();
}

void AiMonster::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<AiType>>(AiType::MONSTER));
	zip.putInt(moveCount);
}

//====
// how many turns the monster chases the player
// after losing his sight
void AiMonster::moveOrAttack(Actor& owner, int targetx, int targety)
{
	int dx = targetx - owner.posX; // get the x distance
	int dy = targety - owner.posY; // get the y distance

	const int stepdx = (dx > 0 ? 1 : -1); // get the x step
	const int stepdy = (dy > 0 ? 1 : -1); // get the y step

	const double distance = sqrt(dx * dx + dy * dy); // get the distance

	if (distance >= 2)
	{
		dx = static_cast<int>(round(dx / distance));
		dy = static_cast<int>(round(dy / distance));

		if (game.map != nullptr)
		{
			if (game.map->can_walk(owner.posX + dx, owner.posY + dy))
			{
				owner.posX += dx;
				owner.posY += dy;
			}
			else if (game.map->can_walk(owner.posX + stepdx, owner.posY))
			{
				owner.posX += stepdx;
			}
			else if (game.map->can_walk(owner.posX, owner.posY + stepdy))
			{
				owner.posY += stepdy;
			}
		}
		else
		{
			std::cout << "Error: game.map is null" << std::endl;
			exit(-1);
		}

	}

	else if (owner.attacker)
	{
		owner.attacker->attack(owner, *game.player);
	}
}
