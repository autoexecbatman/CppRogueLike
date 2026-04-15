#pragma once

#include <string>

#include "../Map/DungeonRoom.h"

struct GameContext;

// A monster candidate for the knapsack solver.
// key matches a MonsterCreator registry key. xpCost is the budget weight.
struct MonsterCandidate
{
    std::string key;
    int xpCost;
};

// Plan and spawn an encounter for the given room.
// Budget is derived from room type and dungeon level.
// Uses 0/1 knapsack to maximize monster variety within budget.
// Hard cap: MAX_ENCOUNTER_MONSTERS monsters per room.
void plan_encounter(const DungeonRoom& room, GameContext& ctx);
