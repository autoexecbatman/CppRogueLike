#include <limits>
#include <vector>

#include "../Actor/Actor.h"
#include "../Actor/Attacker.h"
#include "../Actor/Destructible.h"
#include "../ActorTypes/Player.h"
#include "../Core/GameContext.h"
#include "../Map/Map.h"
#include "../Persistent/Persistent.h"
#include "../Utils/Vector2D.h"
#include "Ai.h"
#include "AiMonster.h"

static const std::vector<Vector2D> NEIGHBORS = {
	DIR_N, DIR_S, DIR_W, DIR_E, DIR_NW, DIR_NE, DIR_SW, DIR_SE
};

// AD&D 2e: Move away from player using inverted Dijkstra gradient.
void AiMonster::flee(Creature& owner, GameContext& ctx)
{
	auto is_occupied = [&ctx](const Vector2D& pos)
	{
		return ctx.map->get_actor(pos, ctx) != nullptr;
	};

	const int currentCost = ctx.map->get_dijkstra_cost(owner.position);

	// Disconnected tile (unreachable from player) — costs are meaningless, hold position.
	if (currentCost == std::numeric_limits<int>::max())
	{
		return;
	}

	Vector2D bestStep{ -1, -1 };
	int bestCost = currentCost; // only accept tiles strictly further than current position

	for (const Vector2D& delta : NEIGHBORS)
	{
		Vector2D candidate{ owner.position.x + delta.x, owner.position.y + delta.y };
		if (!ctx.map->is_in_bounds(candidate))
		{
			continue;
		}
		if (!ctx.map->can_walk(candidate, ctx))
		{
			continue;
		}
		int candidateCost = ctx.map->get_dijkstra_cost(candidate);
		if (candidateCost > bestCost && !is_occupied(candidate))
		{
			bestCost = candidateCost;
			bestStep = candidate;
		}
	}

	if (bestStep.x != -1)
	{
		owner.position = bestStep;
	}
	else
	{
		// At escape apex — no tile further from player is available.
		// AD&D 2e: fight back only if the threat is adjacent; otherwise hold ground.
		if (owner.get_tile_distance(ctx.player->position) <= 1)
		{
			owner.remove_state(ActorState::IS_FLEEING);
			owner.attacker->attack(*ctx.player, ctx);
		}
		// else: hold position, keep IS_FLEEING — player has not cornered us yet
	}
}

// AD&D 2e: Roll 2d10 against morale score; set IS_FLEEING on failure.
void AiMonster::check_morale(Creature& owner, GameContext& ctx)
{
	if (owner.has_state(ActorState::IS_FLEEING))
	{
		return;
	}

	const int hp = owner.destructible->get_hp();
	const int hpMax = owner.destructible->get_max_hp();

	if (hp * 2 > hpMax)
	{
		return;
	}

	const int moraleRoll = ctx.dice->roll(1, 10) + ctx.dice->roll(1, 10);
	if (moraleRoll > owner.get_morale())
	{
		owner.add_state(ActorState::IS_FLEEING);
	}
}

// AD&D 2e: Returns true if the player's Sanctuary spell blocks this monster's turn.
bool AiMonster::blocked_by_sanctuary(GameContext& ctx)
{
	if (!ctx.player->has_state(ActorState::IS_PROTECTED))
	{
		return false;
	}
	return ctx.dice->roll(1, 20) < 15;
}

void AiMonster::move_or_attack(Creature& owner, Vector2D targetPosition, GameContext& ctx)
{
	check_morale(owner, ctx);

	if (owner.has_state(ActorState::IS_FLEEING))
	{
		flee(owner, ctx);
		return;
	}

	int distanceToTarget = owner.get_tile_distance(targetPosition);
	if (distanceToTarget <= 1)
	{
		owner.attacker->attack(*ctx.player, ctx);
		return;
	}

	auto is_blocked = [&ctx](const Vector2D& pos)
	{
		return ctx.map->get_actor(pos, ctx) != nullptr
			|| ctx.map->find_decoration_at(pos, ctx) != nullptr;
	};

	Vector2D bestStep{ -1, -1 };
	int bestCost = std::numeric_limits<int>::max();

	for (const Vector2D& delta : NEIGHBORS)
	{
		Vector2D candidate{ owner.position.x + delta.x, owner.position.y + delta.y };
		if (!ctx.map->is_in_bounds(candidate))
		{
			continue;
		}
		if (!ctx.map->can_walk(candidate, ctx))
		{
			continue;
		}
		int candidateCost = ctx.map->get_dijkstra_cost(candidate);
		if (candidateCost < bestCost && !is_blocked(candidate))
		{
			bestCost = candidateCost;
			bestStep = candidate;
		}
	}

	if (bestStep.x != -1)
	{
		owner.position = bestStep;
	}
}

void AiMonster::update(Creature& owner, GameContext& ctx)
{

	// If the owner has no AI or is dead, do nothing
	if (owner.ai == nullptr || owner.destructible->is_dead())
	{
		return;
	}

	// Sleeping or paralyzed creatures skip their turn
	if (owner.has_state(ActorState::IS_SLEEPING) || owner.has_state(ActorState::IS_HELD))
	{
		return;
	}

	if (blocked_by_sanctuary(ctx))
	{
		return;
	}

	// Check if monster is in FOV - if so, set tracking
	if (ctx.map->is_in_fov(owner.position) && !ctx.player->is_invisible())
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
	if (moveCount > 0 && !ctx.player->is_invisible())
	{
		// Recently seen player - actively pursue
		move_or_attack(owner, ctx.player->position, ctx);
	}
	else if (distanceToPlayer <= 15 && !ctx.player->is_invisible())
	{
		// Within a certain range, monsters might still wander toward player
		// even if they don't have direct line of sight
		if (ctx.dice->d6() == 1) // Occasional movement toward player
		{
			move_or_attack(owner, ctx.player->position, ctx);
		}
		else if (ctx.dice->d10() == 1) // Occasional random movement
		{
			if (owner.has_state(ActorState::IS_FLEEING))
			{
				flee(owner, ctx);
			}
			else
			{
				int dx = ctx.dice->roll(-1, 1);
				int dy = ctx.dice->roll(-1, 1);

				if (dx != 0 || dy != 0)
				{
					Vector2D newPos = owner.position + Vector2D{ dx, dy };
					if (ctx.map->can_walk(newPos, ctx) && !ctx.map->get_actor(newPos, ctx))
					{
						owner.position = newPos;
					}
				}
			}
		}
	}
	else if (ctx.dice->d20() == 1) // Very occasional random movement for distant monsters
	{
		if (owner.has_state(ActorState::IS_FLEEING))
		{
			flee(owner, ctx);
		}
		else
		{
			int dx = ctx.dice->roll(-1, 1);
			int dy = ctx.dice->roll(-1, 1);

			if (dx != 0 || dy != 0)
			{
				Vector2D newPos = owner.position + Vector2D{ dx, dy };
				if (ctx.map->can_walk(newPos, ctx) && !ctx.map->get_actor(newPos, ctx))
				{
					owner.position = newPos;
				}
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

// file: AiMonster.cpp
