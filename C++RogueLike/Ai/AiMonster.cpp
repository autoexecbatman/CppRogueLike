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

void AiMonster::load(const json& j)
{
	moveCount = j.at("moveCount").get<int>();
}

void AiMonster::save(json& j)
{
	j["type"] = static_cast<int>(AiType::MONSTER);
	j["moveCount"] = moveCount;
}

//====
// how many turns the monster chases the player
// after losing his sight

void AiMonster::moveOrAttack(Creature& owner, Vector2D targetPosition)
{
	if(game.map->tile_action(owner, game.map->get_tile_type(owner.position)))
	{
		const auto distance = owner.get_tile_distance(targetPosition);
		if (distance > 1)
		{
			game.map->tcodPath->compute(owner.position.x, owner.position.y, targetPosition.x, targetPosition.y);
			int x, y;
			while (game.map->tcodPath->walk(&x, &y, true))
			{
				Vector2D newPos = Vector2D{ y, x };
				Vector2D oldPos = owner.position;
				if (game.map->can_walk(newPos))
				{
					owner.position = newPos;
					break;
				}
				else
				{

					owner.position = oldPos;
					break;
				}
			}
		}
		else
		{
			owner.attacker->attack(owner, *game.player);
		}
	}
}
// file: AiMonster.cpp
