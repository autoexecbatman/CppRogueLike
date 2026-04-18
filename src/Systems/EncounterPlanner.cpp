// file: EncounterPlanner.cpp
#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "../Actor/Creature.h"
#include "../Core/GameContext.h"
#include "../Factories/MonsterCreator.h"
#include "../Map/DungeonRoom.h"
#include "../Map/Map.h"
#include "../Random/RandomDice.h"
#include "../Utils/Vector2D.h"
#include "EncounterPlanner.h"
#include "LevelManager.h"

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr int BASE_XP_PER_LEVEL = 120;
static constexpr int MAX_ENCOUNTER_MONSTERS = 10;

static constexpr float STANDARD_MULTIPLIER = 1.0f;
static constexpr float DANGER_MULTIPLIER = 2.5f;
static constexpr float TREASURE_MULTIPLIER = 1.5f;

// ---------------------------------------------------------------------------
// Budget calculation
// ---------------------------------------------------------------------------

static int calculate_budget(RoomType type, int dungeonLevel)
{
	switch (type)
	{

	case RoomType::ENTRANCE:
	{
		return 0;
	}

	case RoomType::STANDARD:
	{
		return static_cast<int>(dungeonLevel * BASE_XP_PER_LEVEL * STANDARD_MULTIPLIER);
	}

	case RoomType::DANGER:
	{
		return static_cast<int>(dungeonLevel * BASE_XP_PER_LEVEL * DANGER_MULTIPLIER);
	}

	case RoomType::TREASURE:
	{
		return static_cast<int>(dungeonLevel * BASE_XP_PER_LEVEL * TREASURE_MULTIPLIER);
	}

	}

	return 0;
}

// ---------------------------------------------------------------------------
// Two-phase encounter selection:
// Phase 1 — variety: pick one of each affordable candidate in random order
//           until budget exhausted or monster cap hit.
// Phase 2 — fill: spend remaining budget on random duplicates.
//
// Standard DP breaks for diminishing-return variety functions (CLRS Ch16).
// Two-phase approximation produces good variety without DP complexity.
// ---------------------------------------------------------------------------

static std::vector<std::string> select_encounter(
	const std::vector<MonsterCandidate>& candidates,
	int budget,
	int cap,
	RandomDice& rng)
{
	std::vector<std::string> selected;

	// Phase 1: one of each type, random order (candidates already shuffled)
	int remaining = budget;
	for (const auto& candidate : candidates)
	{
		if (static_cast<int>(selected.size()) >= cap)
		{
			break;
		}
		if (candidate.xpCost <= remaining)
		{
			selected.push_back(candidate.key);
			remaining -= candidate.xpCost;
		}
	}

	// Phase 2: fill remaining budget with random duplicates
	// Build affordable subset for fill phase
	std::vector<const MonsterCandidate*> affordable;
	for (const auto& candidate : candidates)
	{
		if (candidate.xpCost <= remaining)
		{
			affordable.push_back(&candidate);
		}
	}

	while (!affordable.empty() && static_cast<int>(selected.size()) < cap)
	{
		const int idx = rng.roll(0, static_cast<int>(affordable.size()) - 1);
		const MonsterCandidate& pick = *affordable[idx];
		if (pick.xpCost > remaining)
		{
			break;
		}
		selected.push_back(pick.key);
		remaining -= pick.xpCost;

		// Rebuild affordable list with updated remaining budget
		affordable.clear();
		for (const auto& candidate : candidates)
		{
			if (candidate.xpCost <= remaining)
			{
				affordable.push_back(&candidate);
			}
		}
	}

	return selected;
}

// ---------------------------------------------------------------------------
// Random walkable position inside room
// ---------------------------------------------------------------------------

static Vector2D random_room_position(const DungeonRoom& room, GameContext& ctx)
{
	Vector2D pos{};
	constexpr int MAX_TRIES = 50;
	for (int attempt = 0; attempt < MAX_TRIES; ++attempt)
	{
		pos.x = ctx.dice->roll(room.col, room.col_end());
		pos.y = ctx.dice->roll(room.row, room.row_end());
		if (ctx.map->can_walk(pos, ctx) &&
			ctx.map->get_actor(pos, ctx) == nullptr &&
			ctx.map->find_decoration_at(pos, ctx) == nullptr)
		{
			return pos;
		}
	}
	return { -1, -1 }; // invalid sentinel
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void plan_encounter(const DungeonRoom& room, GameContext& ctx)
{
	if (!ctx.levelManager || !ctx.dice || !ctx.map || !ctx.creatures)
	{
		return;
	}

	const int dungeonLevel = ctx.levelManager->get_dungeon_level();
	const int baseBudget = calculate_budget(room.type, dungeonLevel);
	const int roll = ctx.dice->roll(60, 100);
	const int budget = baseBudget * roll / 100;

	if (budget <= 0)
	{
		return;
	}

	// Build candidate list: monsters eligible for this dungeon level
	std::vector<MonsterCandidate> candidates;
	for (const auto& key : MonsterCreator::get_all_keys())
	{
		const MonsterParams& params = MonsterCreator::get_params(key);
		if (params.xp <= 0)
		{
			continue;
		}
		if (params.levelMinimum > dungeonLevel)
		{
			continue;
		}
		if (params.levelMaximum > 0 && params.levelMaximum < dungeonLevel)
		{
			continue;
		}
		candidates.push_back({ key, params.xp });
	}

	if (candidates.empty())
	{
		return;
	}

	// Shuffle candidates for variety in phase 1 selection order
	std::ranges::shuffle(candidates, ctx.dice->get_rng());

	// Two-phase selection: variety first, fill second
	std::vector<std::string> selected = select_encounter(
		candidates, budget, MAX_ENCOUNTER_MONSTERS, *ctx.dice);

	// Spawn each selected monster at a random walkable position
	for (const auto& key : selected)
	{
		const Vector2D pos = random_room_position(room, ctx);
		if (pos.x < 0)
		{
			break; // no more walkable positions
		}
		ctx.creatures->push_back(
			MonsterCreator::create_from_params(pos, MonsterCreator::get_params(key), ctx));
	}
}
