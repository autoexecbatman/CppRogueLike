// file: AiMonster.cpp
#include "AiMonster.h"
#include "../Game.h"
#include "../Utils/Dijkstra.h"

//==MONSTER_AI==

AiMonster::AiMonster() : moveCount(0) {}

void AiMonster::update(Creature& owner, GameContext& ctx)
{

    // If the owner has no AI or is dead, do nothing
    if (owner.ai == nullptr || owner.destructible->is_dead())
    {
        return;
    }

    // Check if monster is in FOV - if so, set tracking
    if (ctx.map->is_in_fov(owner.position))
    {
        // Player can see monster - set maximum tracking
        moveCount = TRACKING_TURNS;
    }
    else
    {
        // Player can't see monster
        if (moveCount > 0)
        {
            // If we were previously tracking the player, decrement counter
            moveCount--;
        }
    }

    // Distance to player
    int distanceToPlayer = owner.get_tile_distance(ctx.player->position);

    // Behavior based on distance and visibility
    if (moveCount > 0)
    {
        // Recently seen player - actively pursue
        moveOrAttack(owner, ctx.player->position, ctx);
    }
    else if (distanceToPlayer <= 15)
    {
        // Within a certain range, monsters might still wander toward player
        // even if they don't have direct line of sight
        if (ctx.dice->d6() == 1)  // Occasional movement toward player
        {
            moveOrAttack(owner, ctx.player->position, ctx);
        }
        else if (ctx.dice->d10() == 1)  // Occasional random movement
        {
            // Random movement in one of eight directions
            int dx = ctx.dice->roll(-1, 1);
            int dy = ctx.dice->roll(-1, 1);

            if (dx != 0 || dy != 0)  // Ensure we're not standing still
            {
                Vector2D newPos = owner.position + Vector2D{ dy, dx };
                if (ctx.map->can_walk(newPos, ctx) && !ctx.map->get_actor(newPos, ctx))
                {
                    owner.position = newPos;
                }
            }
        }
    }
    else if (ctx.dice->d20() == 1)  // Very occasional random movement for distant monsters
    {
        // Random movement in one of eight directions
        int dx = ctx.dice->roll(-1, 1);
        int dy = ctx.dice->roll(-1, 1);

        if (dx != 0 || dy != 0)  // Ensure we're not standing still
        {
            Vector2D newPos = owner.position + Vector2D{ dy, dx };
            if (ctx.map->can_walk(newPos, ctx) && !ctx.map->get_actor(newPos, ctx))
            {
                owner.position = newPos;
            }
        }
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

//void AiMonster::moveOrAttack(Creature& owner, Vector2D targetPosition)
//{
//	if(ctx.map->tile_action(owner, ctx.map->get_tile_type(owner.position)))
//	{
//		Vector2D moveDirection = targetPosition - owner.position;
//
//		Vector2D diagonalMove = Vector2D{
//			calculate_step(moveDirection.y),
//			calculate_step(moveDirection.x)
//		};
//
//		std::vector<Vector2D> moves = {
//			diagonalMove,
//			Vector2D{0, diagonalMove.x},
//			Vector2D{diagonalMove.y, 0}
//		};
//
//		const auto distance = owner.get_tile_distance(targetPosition);
//		if (distance > 1)
//		{
//			ctx.map->tcodPath->compute(owner.position.x, owner.position.y, targetPosition.x, targetPosition.y);
//			int x, y;
//			while (ctx.map->tcodPath->walk(&x, &y, true))
//			{
//				Vector2D newPos = Vector2D{ y, x };
//				Vector2D oldPos = owner.position;
//				if (ctx.map->can_walk(newPos))
//				{
//					owner.position = newPos;
//					break;
//				}
//				else
//				{
//					for (const auto& m : moves)
//					{
//						Vector2D moveTo = owner.position + m;
//						if (ctx.map->can_walk(moveTo))
//						{
//							owner.position = moveTo;
//						}
//					}
//					break;
//				}
//			}
//		}
//		else
//		{
//			owner.attacker->attack(owner, *ctx.player);
//		}
//	}
//}

void AiMonster::moveOrAttack(Creature& owner, Vector2D targetPosition, GameContext& ctx)
{
	Dijkstra dijkstra{ MAP_WIDTH,MAP_HEIGHT };
	std::vector<Vector2D> path = dijkstra.a_star_search(*ctx.map, owner.position, targetPosition, false, ctx);
	int distanceToTarget = owner.get_tile_distance(targetPosition);
	auto is_actor = [&ctx](const Vector2D& pos) { return ctx.map->get_actor(pos, ctx) != nullptr; };
	if (distanceToTarget > 1)
	{
		if (!path.empty())
		{
			if(!is_actor(path.at(1)))
			{
				owner.position = path.at(1);
			}
		}
	}
	else
	{
		owner.attacker->attack(owner, *ctx.player, ctx);
	}
}

// file: AiMonster.cpp
