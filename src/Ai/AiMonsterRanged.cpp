#include <algorithm>
#include <cmath>
#include <functional>

#include "../Actor/Actor.h"
#include "../Actor/Attacker.h"
#include "../Actor/Creature.h"
#include "../Actor/Destructible.h"
#include "../Core/GameContext.h"
#include "../Map/Map.h"
#include "../Persistent/Persistent.h"
#include "../Systems/AnimationSystem.h"
#include "../Systems/TileConfig.h"
#include "../Utils/Vector2D.h"
#include "AiMonster.h"
#include "AiMonsterRanged.h"

void AiMonsterRanged::update(Creature& owner, GameContext& ctx)
{
	if (owner.destructible->is_dead())
	{
		return;
	}

	if (ctx.map->is_in_fov(owner.position))
	{
		// Move towards the player if we can see them
		moveCount = TRACKING_TURNS;
	}
	else
	{
		moveCount--;
	}

	if (blocked_by_sanctuary(ctx))
	{
		return;
	}

	if (moveCount > 0)
	{
		move_or_attack(owner, ctx.player->position, ctx);
	}
}

void AiMonsterRanged::move_or_attack(Creature& owner, Vector2D targetPosition, GameContext& ctx)
{
	int distance = owner.get_tile_distance(targetPosition);

	// If at optimal range and have line of sight, try a ranged attack
	if (distance <= maxRangeDistance && distance >= 2 && ctx.map->has_los(owner.position, targetPosition))
	{
		if (tryRangedAttack(owner, targetPosition, ctx))
		{
			return; // Attack succeeded, end turn
		}
	}

	// If too close, try to back away
	if (distance < optimalDistance)
	{
		// Calculate direction away from player
		Vector2D moveDir = owner.position - targetPosition;

		// Normalize and scale
		int maxComponent = std::max(std::abs(moveDir.x), std::abs(moveDir.y));
		if (maxComponent > 0)
		{
			moveDir.x = moveDir.x / maxComponent;
			moveDir.y = moveDir.y / maxComponent;
		}

		// Try to move away in this direction
		Vector2D newPos = owner.position + moveDir;
		if (ctx.map->can_walk(newPos, ctx) && !ctx.map->get_actor(newPos, ctx))
		{
			owner.position = newPos;
			return;
		}
	}

	// If too far or couldn't back up, use regular pathfinding to approach
	if (distance > maxRangeDistance || distance < optimalDistance)
	{
		AiMonster::move_or_attack(owner, targetPosition, ctx);
	}
}

bool AiMonsterRanged::tryRangedAttack(Creature& owner, Vector2D targetPos, GameContext& ctx)
{
	// Check if there's a clear line of sight
	if (!ctx.map->has_los(owner.position, targetPos))
	{
		return false;
	}

	animate_arrow(owner.position, targetPos, ctx);

	// Perform the attack
	owner.attacker->attack(*ctx.player, ctx);
	return true;
}

void AiMonsterRanged::animate_arrow(Vector2D from, Vector2D to, GameContext& ctx)
{
	if (!ctx.animSystem || !ctx.tileConfig)
	{
		return;
	}

	const TileRef boltTile = ctx.tileConfig->get("TILE_EFFECT_BOLT");

	auto onArrive = [to, &ctx]()
	{
		if (ctx.animSystem)
		{
			ctx.animSystem->spawn_blood_burst(to.x, to.y, 3);
		}
	};

	ctx.animSystem->spawn_projectile(
		from,
		to,
		boltTile,
		210, 180, 100,
		550.0f,
		0.0f,
		std::move(onArrive));
}

void AiMonsterRanged::load(const json& j)
{
	AiMonster::load(j);
	// Additional ranged AI properties can be loaded here
	if (j.contains("maxRangeDistance"))
	{
		maxRangeDistance = j.at("maxRangeDistance").get<int>();
	}
	if (j.contains("optimalDistance"))
	{
		optimalDistance = j.at("optimalDistance").get<int>();
	}
}

void AiMonsterRanged::save(json& j)
{
	AiMonster::save(j);
	// Additional ranged AI properties
	j["maxRangeDistance"] = maxRangeDistance;
	j["optimalDistance"] = optimalDistance;
}
