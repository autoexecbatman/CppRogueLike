#include <limits>
#include <vector>

#include "../Actor/Actor.h"
#include "../Actor/Attacker.h"
#include "../Actor/Creature.h"
#include "../Core/GameContext.h"
#include "../Map/Map.h"
#include "../Persistent/Persistent.h"
#include "../Utils/Vector2D.h"
#include "Ai.h"
#include "AiMonster.h"

namespace
{
	const std::vector<Vector2D> NEIGHBORS = {
		DIR_N, DIR_S, DIR_W, DIR_E, DIR_NW, DIR_NE, DIR_SW, DIR_SE
	};

	// AD&D 2e: Move away from player using inverted Dijkstra gradient.
	void flee(Creature& owner, GameContext& ctx)
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

		// TODO: bestStep uses x==-1 as sentinel for "no step found"; replace with std::optional<Vector2D>
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
	void check_morale(Creature& owner, GameContext& ctx)
	{
		if (owner.has_state(ActorState::IS_FLEEING))
		{
			return;
		}

		const int hp = owner.get_hp();
		const int hpMax = owner.get_max_hp();

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

	// One step of random drift. No-ops when the chosen tile is blocked or occupied.
	void random_wander(Creature& owner, GameContext& ctx)
	{
		int dx = ctx.dice->roll(-1, 1);
		int dy = ctx.dice->roll(-1, 1);
		if (dx == 0 && dy == 0)
		{
			return;
		}
		Vector2D newPos = owner.position + Vector2D{ dx, dy };
		if (ctx.map->can_walk(newPos, ctx) && !ctx.map->get_actor(newPos, ctx))
		{
			owner.position = newPos;
		}
	}
	// Returns true when this creature must skip its turn entirely.
	bool cannot_act(Creature& owner, GameContext& ctx)
	{
		if (owner.ai == nullptr || owner.is_dead())
		{
			return true;
		}
		if (owner.has_state(ActorState::IS_SLEEPING) || owner.has_state(ActorState::IS_HELD))
		{
			return true;
		}
		return blocked_by_sanctuary(ctx);
	}
} // namespace

// AD&D 2e: Returns true if the player's Sanctuary spell blocks this monster's turn.
bool blocked_by_sanctuary(GameContext& ctx)
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

	// TODO: bestStep uses x==-1 as sentinel for "no step found"; replace with std::optional<Vector2D>
	if (bestStep.x != -1)
	{
		owner.position = bestStep;
	}
}

// Keeps moveCount current: full reset when player is visible, decay when not.
void AiMonster::update_tracking(Creature& owner, const GameContext& ctx)
{
	if (ctx.map->is_in_fov(owner.position) && !ctx.player->is_invisible())
	{
		moveCount = TRACKING_TURNS;
	}
	else if (moveCount > 0)
	{
		--moveCount;
	}
}

// AD&D 2e behavior dispatch: flee always wins, then pursue, then wander.
void AiMonster::decide_action(Creature& owner, GameContext& ctx)
{
	// A fleeing creature flees every turn — never gated behind wander dice.
	if (owner.has_state(ActorState::IS_FLEEING))
	{
		flee(owner, ctx);
		return;
	}

	int distanceToPlayer = owner.get_tile_distance(ctx.player->position);

	if (moveCount > 0 && !ctx.player->is_invisible())
	{
		move_or_attack(owner, ctx.player->position, ctx);
	}
	else if (distanceToPlayer <= 15 && !ctx.player->is_invisible())
	{
		if (ctx.dice->d6() == 1)
		{
			move_or_attack(owner, ctx.player->position, ctx);
		}
		else if (ctx.dice->d10() == 1)
		{
			random_wander(owner, ctx);
		}
	}
	else if (ctx.dice->d20() == 1)
	{
		random_wander(owner, ctx);
	}
}

void AiMonster::update(Creature& owner, GameContext& ctx)
{
	if (cannot_act(owner, ctx))
	{
		return;
	}
	update_tracking(owner, ctx);
	decide_action(owner, ctx);
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
