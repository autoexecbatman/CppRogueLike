#include <algorithm>
#include <cassert>
#include <cmath>

#include "Actor.h"
#include "AiMonster.h"
#include "AiMonsterRanged.h"
#include "Attacker.h"
#include "BuffSystem.h"
#include "Creature.h"
#include "GameContext.h"
#include "Map.h"
#include "Pathfinding.h"
#include "Persistent.h"
#include "Vector2D.h"

void AiMonsterRanged::update(Creature& owner, GameContext& ctx)
{
	if (owner.is_dead())
	{
		return;
	}

	owner.update_awareness(ctx);

	// A failed save against Sanctuary means this creature has lost track of the player
	// entirely (PHB page 436), so there is nothing here to shoot at or walk toward.
	if (owner.is_aware(ctx.gameState->get_time()) && !ctx.buffSystem->ignores_warded_creature(owner, *ctx.player()))
	{
		move_or_attack(owner, ctx.player()->position, ctx);
	}
}

void AiMonsterRanged::move_or_attack(Creature& owner, Vector2D targetPosition, GameContext& ctx)
{
	assert(ctx.map && "AiMonsterRanged::move_or_attack: called without a map");

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

		// Try to move away in this direction; a step that did anything spends the turn.
		const Pathfinding::StepResult backedAway = Pathfinding::step(owner, owner.position + moveDir, ctx);
		if (backedAway != Pathfinding::StepResult::REFUSED)
		{
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
	assert(ctx.map && "AiMonsterRanged::tryRangedAttack: called without a map");

	// Check if there's a clear line of sight
	if (!ctx.map->has_los(owner.position, targetPos))
	{
		return false;
	}

	RangedShot::fire(owner, *ctx.player(), ctx);
	return true;
}

void AiMonsterRanged::load(const json& source)
{
	AiMonster::load(source);
	maxRangeDistance = source.at("maxRangeDistance").get<int>();
	optimalDistance = source.at("optimalDistance").get<int>();
}

void AiMonsterRanged::save(json& output)
{
	AiMonster::save(output);
	// Additional ranged AI properties
	output["maxRangeDistance"] = maxRangeDistance;
	output["optimalDistance"] = optimalDistance;
}
