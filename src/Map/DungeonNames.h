#pragma once

#include <string>

#include "../Random/RandomDice.h"

// Procedural name generation for named dungeon NPCs.
// Each function takes a seeded RandomDice so names are deterministic per map seed.
namespace DungeonNames
{
// Returns a unique boss name in "[FirstName] [Epithet]" format.
// 15 first names x 15 epithets = 225 distinct combinations.
std::string generate_warden_name(RandomDice& rng);
}
