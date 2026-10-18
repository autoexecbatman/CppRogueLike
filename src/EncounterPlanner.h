#pragma once

#include <string>
#include <vector>

#include "DungeonRoom.h"

class MonsterRegistry;
struct GameContext;

// A monster candidate for the knapsack solver.
// key matches a MonsterCreator registry key. xpCost is the budget weight.
struct MonsterCandidate
{
	std::string key;
	int xpCost;
};

class RandomDice;

// The experience a room of this type is worth spending at this depth. An entrance
// is worth nothing, so nothing is placed there.
int encounter_budget(RoomType type, int dungeonLevel);

// Which monsters to place, given what they cost and what may be spent. One of each
// affordable kind first, in the order given, then random duplicates until the budget
// or the cap runs out. Never spends more than the budget and never returns more than
// the cap.
std::vector<std::string> select_encounter(
	const std::vector<MonsterCandidate>& candidates,
	int budget,
	int cap,
	RandomDice& rng);

// The monsters an encounter at this depth may draw on, each at the experience its budget
// pays: the highest count's, so a budget covers the worst roll. A monster worth nothing, or
// outside its depth range, is left out.
//
// Example, a registry holding a monster of 9 or 10 hit dice worth 2,000 or 3,000:
//   encounter_candidates(monsters, 1); // -> includes { "ranged_count_beast", 3000 }, the worst roll
std::vector<MonsterCandidate> encounter_candidates(const MonsterRegistry& monsters, int dungeonLevel);

// Plan and spawn an encounter for the given room.
// Budget is derived from room type and dungeon level.
// Uses 0/1 knapsack to maximize monster variety within budget.
// Hard cap: MAX_ENCOUNTER_MONSTERS monsters per room.
void plan_encounter(const DungeonRoom& room, GameContext& ctx);
