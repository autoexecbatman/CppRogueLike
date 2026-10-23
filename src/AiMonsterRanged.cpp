#include <algorithm>
#include <cassert>
#include <cmath>

#include "Actor.h"
#include "AiMonster.h"
#include "AiMonsterRanged.h"
#include "Attacker.h"
#include "BuffSystem.h"
#include "Creature.h"
#include "EquipmentSlot.h"
#include "GameContext.h"
#include "Item.h"
#include "Map.h"
#include "MonsterSpellcasting.h"
#include "Pathfinding.h"
#include "Persistent.h"
#include "Pickable.h"
#include "Vector2D.h"

namespace
{

// Whether the creature holds a missile weapon its Strength, as it stands now, can draw.
// Asked every turn, because Strength can fall between one turn and the next.
//
// Example:
//   can_draw_missile_weapon(strength18WithCompositeBow);  // -> true
//   can_draw_missile_weapon(strength12WithCompositeBow);  // -> false, the composite bow needs 18
//   can_draw_missile_weapon(archerWithNoBow);             // -> false, nothing to shoot with
bool can_draw_missile_weapon(const Creature& owner)
{
	const Item* missileWeapon =
		owner.get_equipped_item(EquipmentSlot::MISSILE_WEAPON);
	return missileWeapon && can_draw(owner, *missileWeapon);
}

} // namespace

void AiMonsterRanged::update(Creature& owner, GameContext& ctx)
{
	if (owner.is_dead())
	{
		return;
	}

	owner.update_awareness(ctx);

	// A failed save against Sanctuary means this creature has lost track of the player
	// entirely (PHB page 436), so there is nothing here to shoot at or walk toward.
	if (owner.is_aware(ctx.gameState->get_time()) &&
		!ctx.buffSystem->ignores_warded_creature(owner, *ctx.player()))
	{
		// A spell that reaches is the turn; only without one does the archer shoot or close.
		if (MonsterSpellcasting::try_cast_at(owner, *ctx.player(), ctx) ==
			MonsterCast::NOTHING_CAST)
		{
			move_or_attack(owner, *ctx.player(), ctx);
		}
	}
}

void AiMonsterRanged::move_or_attack(
	Creature& owner,
	Creature& target,
	GameContext& ctx)
{
	assert(ctx.map && "AiMonsterRanged::move_or_attack: called without a map");

	// An archer with nothing it can draw closes in and fights as any monster does.
	if (!can_draw_missile_weapon(owner))
	{
		AiMonster::move_or_attack(owner, target, ctx);
		return;
	}

	int distance = owner.get_tile_distance(target.position);

	// If at optimal range and have line of sight, try a ranged attack
	if (distance <= maxRangeDistance && distance >= 2 &&
		ctx.map->has_los(owner.position, target.position))
	{
		if (try_ranged_attack(owner, target, ctx))
		{
			return; // Attack succeeded, end turn
		}
	}

	// If too close, try to back away
	if (distance < optimalDistance)
	{
		// Calculate direction away from player
		Vector2D retreatDirection = owner.position - target.position;

		// Normalize and scale
		int maxComponent = std::max(
			std::abs(retreatDirection.x),
			std::abs(retreatDirection.y));
		if (maxComponent > 0)
		{
			retreatDirection.x = retreatDirection.x / maxComponent;
			retreatDirection.y = retreatDirection.y / maxComponent;
		}

		// Try to move away in this direction; a step that did anything spends the turn.
		const Pathfinding::StepResult backedAway =
			Pathfinding::step(owner, owner.position + retreatDirection, ctx);
		if (backedAway != Pathfinding::StepResult::REFUSED)
		{
			return;
		}
	}

	// If too far or couldn't back up, use regular pathfinding to approach
	if (distance > maxRangeDistance || distance < optimalDistance)
	{
		AiMonster::move_or_attack(owner, target, ctx);
	}
}

bool AiMonsterRanged::try_ranged_attack(
	Creature& owner,
	Creature& target,
	GameContext& ctx)
{
	assert(
		ctx.map && "AiMonsterRanged::try_ranged_attack: called without a map");

	// Check if there's a clear line of sight
	if (!ctx.map->has_los(owner.position, target.position))
	{
		return false;
	}

	RangedShot::fire(owner, target, ctx);
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
