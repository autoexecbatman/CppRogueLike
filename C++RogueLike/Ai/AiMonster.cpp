// file: AiMonster.cpp
#include "AiMonster.h"
#include "../Game.h"

//==MONSTER_AI==

constexpr auto TRACKING_TURNS = 3; // Used in AiMonster::update()

AiMonster::AiMonster() : moveCount(0) {}

void AiMonster::update(Creature& owner)
{
	game.log(owner.actorData.name + "AI is updating");
	if (owner.ai == nullptr) // if the owner has no ai
	{
		return; // do nothing
	}

	if (owner.destructible->is_dead()) // if the owner is dead
	{
		return; // do nothing
	}

	if (game.map->is_in_fov(owner.position)) // if the owner is in the fov
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
		moveOrAttack(owner, game.player->position); // move or attack the player
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
void AiMonster::moveOrAttack(Creature& owner, Vector2D targetPosition)
{
	Vector2D target = targetPosition - owner.position;

	int stepX = target.x > 0 ? 1 : -1;
	int stepY = target.y > 0 ? 1 : -1;

	const double distance = sqrt(target.x * target.x + target.y * target.y); // get the distance

	if (distance >= 2)
	{
		target.x = static_cast<int>(round(target.x / distance));
		target.y = static_cast<int>(round(target.y / distance));

		if (game.map->can_walk(owner.position + target))
		{
			owner.position.x += target.x;
			owner.position.y += target.y;
		}
		else if (game.map->can_walk(Vector2D{ owner.position.y, owner.position.x + stepX }))
		{
			owner.position.x += stepX;
		}
		else if (game.map->can_walk(Vector2D{ owner.position.y + stepY, owner.position.x }))
		{
			owner.position.y += stepY;
		}
	}

	else if (owner.attacker)
	{
		owner.attacker->attack(owner, *game.player);
	}
}

// file: AiMonster.cpp
