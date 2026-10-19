#include <cassert>
#include <optional>
#include <vector>

#include "Actor.h"
#include "Ai.h"
#include "AiMonster.h"
#include "AttackKind.h"
#include "Attacker.h"
#include "BuffSystem.h"
#include "Creature.h"
#include "GameContext.h"
#include "Map.h"
#include "Pathfinding.h"
#include "Persistent.h"
#include "Vector2D.h"

namespace
{
const std::vector<Vector2D> NEIGHBORS = {
	DIR_N, DIR_S, DIR_W, DIR_E, DIR_NW, DIR_NE, DIR_SW, DIR_SE
};

// AD&D 2e: Move away from player, up the cost map this creature would descend to reach it.
void flee(Creature& owner, GameContext& ctx)
{
	assert(ctx.pathfinder && "flee: called without a pathfinder");
	assert(ctx.tileConfig && "flee: called without a tileConfig");
	assert(ctx.map && "flee: called without a map");

	auto is_occupied = [&ctx](const Vector2D& pos)
	{
		return ctx.map->get_actor(pos, ctx) != nullptr;
	};

	const Vector2D quarry = ctx.player()->position;
	const Pathfinding::Mobility mobility = Pathfinding::mobility_of(owner, *ctx.tileConfig);
	const std::optional<int> currentCost = ctx.pathfinder->cost_to(quarry, owner.position, mobility, ctx);

	// Disconnected tile (unreachable from player): costs are meaningless, hold position.
	if (!currentCost)
	{
		return;
	}

	std::optional<Vector2D> bestStep;
	int bestCost = *currentCost; // only accept tiles strictly further than current position

	// A tile this creature may not enter is one its own cost map never reached.
	for (const Vector2D& delta : NEIGHBORS)
	{
		Vector2D candidate{ owner.position.x + delta.x, owner.position.y + delta.y };
		const std::optional<int> candidateCost = ctx.pathfinder->cost_to(quarry, candidate, mobility, ctx);
		if (candidateCost && *candidateCost > bestCost && !is_occupied(candidate))
		{
			bestCost = *candidateCost;
			bestStep = candidate;
		}
	}

	if (bestStep)
	{
		Pathfinding::step(owner, *bestStep, ctx);
	}
	else
	{
		// At escape apex - no tile further from player is available.
		// AD&D 2e: fight back only if the threat is adjacent; otherwise hold ground.
		if (owner.get_tile_distance(ctx.player()->position) <= 1)
		{
			owner.remove_state(ActorState::IS_FLEEING);
			owner.attacker->attack(*ctx.player(), AttackKind::MELEE, ctx);
		}
		// else: hold position, keep IS_FLEEING - player has not cornered us yet
	}
}

// AD&D 2e: Roll 2d10 against morale score; set IS_FLEEING on failure.
void check_morale(Creature& owner, GameContext& ctx)
{
	assert(ctx.dice && "check_morale: called without dice");

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
	assert(ctx.dice && "random_wander: called without dice");

	int deltaCol = ctx.dice->roll(-1, 1);
	int deltaRow = ctx.dice->roll(-1, 1);
	if (deltaCol == 0 && deltaRow == 0)
	{
		return;
	}
	Pathfinding::step(owner, owner.position + Vector2D{ deltaCol, deltaRow }, ctx);
}

// With no way through to the quarry, a creature still comes as close as the ground lets
// it, so it waits at the bank of a moat rather than wherever it stood.
void close_in_without_a_route(Creature& owner, Vector2D targetPosition, const Pathfinding::Mobility& mobility, GameContext& ctx)
{
	assert(ctx.map && "close_in_without_a_route: called without a map");

	std::optional<Vector2D> bestStep;
	int bestDistance = owner.get_tile_distance(targetPosition);

	for (const Vector2D& delta : NEIGHBORS)
	{
		const Vector2D candidate = owner.position + delta;
		if (!Pathfinding::step_cost(candidate, mobility, ctx) || ctx.map->get_actor(candidate, ctx) != nullptr)
		{
			continue;
		}
		const int distance = candidate.chebyshev_distance_to(targetPosition);
		if (distance < bestDistance)
		{
			bestDistance = distance;
			bestStep = candidate;
		}
	}

	if (bestStep)
	{
		Pathfinding::step(owner, *bestStep, ctx);
	}
}

// Returns true when this creature must skip its turn entirely.
bool cannot_act(Creature& owner)
{
	if (owner.ai == nullptr || owner.is_dead())
	{
		return true;
	}
	return owner.has_state(ActorState::IS_SLEEPING) || owner.has_state(ActorState::IS_HELD);
}
} // namespace

void AiMonster::move_or_attack(Creature& owner, Vector2D targetPosition, GameContext& ctx)
{
	assert(ctx.pathfinder && "AiMonster::move_or_attack: called without a pathfinder");
	assert(ctx.map && "AiMonster::move_or_attack: called without a map");

	check_morale(owner, ctx);

	if (owner.has_state(ActorState::IS_FLEEING))
	{
		flee(owner, ctx);
		return;
	}

	int distanceToTarget = owner.get_tile_distance(targetPosition);
	if (distanceToTarget <= 1)
	{
		owner.attacker->attack(*ctx.player(), AttackKind::MELEE, ctx);
		return;
	}

	assert(ctx.tileConfig && "move_or_attack: a monster hunted with no tile configuration");
	const Pathfinding::Mobility mobility = Pathfinding::mobility_of(owner, *ctx.tileConfig);

	// The cost map ignores creatures, so it says whether terrain allows any way at all.
	const std::optional<int> holdingCost = ctx.pathfinder->cost_to(targetPosition, owner.position, mobility, ctx);
	if (!holdingCost)
	{
		close_in_without_a_route(owner, targetPosition, mobility, ctx);
		return;
	}

	// The route goes round the creatures standing now. Each step shortens it while they
	// stand still, so a crowd cannot hold the hunter pacing between equal tiles.
	const std::vector<Vector2D> route = ctx.pathfinder->find_route(owner.position, targetPosition, mobility, ctx);
	std::optional<Vector2D> nextStep = route.size() >= 2 ? std::optional<Vector2D>{ route[1] } : std::nullopt;

	// The crowd has closed every way: come up behind it, downhill only, so each step is
	// strictly nearer and none can be undone by the next.
	if (!nextStep)
	{
		int bestCost = *holdingCost;
		for (const Vector2D& delta : NEIGHBORS)
		{
			const Vector2D candidate = owner.position + delta;
			const std::optional<int> candidateCost = ctx.pathfinder->cost_to(targetPosition, candidate, mobility, ctx);
			if (candidateCost && *candidateCost < bestCost && ctx.map->get_actor(candidate, ctx) == nullptr)
			{
				bestCost = *candidateCost;
				nextStep = candidate;
			}
		}
	}

	if (!nextStep)
	{
		return;
	}

	// A shut door on the way is opened this turn and walked through the next.
	Pathfinding::step(owner, *nextStep, ctx);
}

// Keeps the creature's awareness current for this turn.
void AiMonster::update_tracking(Creature& owner, const GameContext& ctx)
{
	owner.update_awareness(ctx);
}

// One creature's turn, chosen from three behaviours: flee, hunt, or drift.
//
// Flee outranks everything, so a routed creature is never held up behind the wander
// dice. Hunting needs all three of awareness, a player who is not invisible, and no
// failed save against Sanctuary; losing any one of them drops the turn to the dice.
//
// Awareness is memory of sight: update_awareness books AWARENESS_ROUNDS whenever the
// creature stands in the player's field of view. The second branch is therefore the
// out-of-sight case, and it still paths at the player's exact position on a d6 of 1,
// with no line of sight tested.
//
// The nested dice resolved, so no reader has to work them out again:
//
//   unseen, inside 15 tiles    hunt 1/6     drift 1/12    hold 3/4
//   no quarry at all           hunt never   drift 1/20    hold 19/20
//
// That 1/6 compounds. An unseen creature has closed in at least once with probability
// 0.42 after three turns, 0.60 after five, and 0.84 after ten.
//
// Example, a goblin around a corner at (10,2) with the player at (4,5), outside the
// player's field of view and with its awareness expired:
//
//   in_player_fov=0 is_aware=0 distance=6
//   d6=1        -> pos=(9,2)  distance=5    // paths at a player it cannot see
//   d6=2,d10=1  -> pos=(9,2)  distance=5    // drifts, direction straight off the dice
//   d6=2,d10=2  -> pos=(10,2) distance=6    // stands still
//
// Both moving branches reach (9,2) because that dead end offers one legal step. The
// first picks it off the cost map and the second off a die.
void AiMonster::decide_action(Creature& owner, GameContext& ctx)
{
	assert(ctx.dice && "AiMonster::decide_action: called without dice");

	// Routed creatures run before any die is read.
	if (owner.has_state(ActorState::IS_FLEEING))
	{
		flee(owner, ctx);
		return;
	}

	// Chebyshev tiles. Only the second branch reads it.
	int distanceToPlayer = owner.get_tile_distance(ctx.player()->position);

	// A failed save against Sanctuary means this creature ignores the warded player (PHB page 436).
	// Reading the record costs no save: a save is owed only to an opponent already swinging.
	const bool hasLostTrackOfPlayer = ctx.buffSystem->ignores_warded_creature(owner, *ctx.player());

	// Perceived: hunt, with no die in the way.
	if (owner.is_aware(ctx.gameState->get_time()) && !ctx.player()->is_invisible() && !hasLostTrackOfPlayer)
	{
		move_or_attack(owner, ctx.player()->position, ctx);
	}
	// Unperceived and inside 15 tiles: 1/6 of turns hunt anyway, aimed at the player's
	// exact position, through walls and around corners alike.
	else if (distanceToPlayer <= 15 && !ctx.player()->is_invisible() && !hasLostTrackOfPlayer)
	{
		if (ctx.dice->d6() == 1)
		{
			move_or_attack(owner, ctx.player()->position, ctx);
		}
		// The d6 missed: a tenth of the remainder drifts, which is 1/12 of all turns,
		// and the other 3/4 hold position.
		else if (ctx.dice->d10() == 1)
		{
			random_wander(owner, ctx);
		}
	}
	// No quarry at all, whether too far, hidden, or warded away: drift on 1/20, and
	// hold position on the other 19/20.
	else if (ctx.dice->d20() == 1)
	{
		random_wander(owner, ctx);
	}
}

void AiMonster::update(Creature& owner, GameContext& ctx)
{
	if (cannot_act(owner))
	{
		return;
	}
	update_tracking(owner, ctx);
	decide_action(owner, ctx);
}

void AiMonster::load([[maybe_unused]] const json& source)
{
}

void AiMonster::save(json& output)
{
	output["type"] = encode_ai_type(get_ai_type());
}

// file: AiMonster.cpp
