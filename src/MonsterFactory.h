#pragma once

#include <string>
#include <vector>

#include "Vector2D.h"

// file: MonsterFactory.h
//
// Draws a random monster for a dungeon level. The table it draws from is derived from
// a MonsterRegistry on every call - the registry's standard and custom monsters with
// the weights they hold at that moment, then the class-based creatures, whose weights
// are written in the factory - so nothing keeps a copy that an edit, a new level or a
// game loaded from a save could find stale or empty.
//
// Usage:
//
//   MonsterFactory::spawn_random_monster(position, dungeonLevel, ctx); // pushes one creature onto ctx.creatures
//   MonsterFactory::get_current_distribution(1, *ctx.monsterRegistry); // -> one { name, percentage } per monster that can appear

struct GameContext;
class MonsterRegistry;

// One monster's chance of being drawn, in percent.
struct MonsterPercentage
{
	// The name the monster is drawn under.
	std::string name{};
	// Its share of the level's total spawn weight, 0 to 100.
	float percentage{};
};

namespace MonsterFactory
{
// Spawns one monster at position, onto ctx.creatures, drawn by weight from
// ctx.monsterRegistry for dungeonLevel. Logs and spawns nothing when no monster can
// appear at that level.
void spawn_random_monster(Vector2D position, int dungeonLevel, GameContext& ctx);

// Each monster's chance at dungeonLevel, from the registry as it stands; a monster that
// cannot appear there is left out.
[[nodiscard]] std::vector<MonsterPercentage> get_current_distribution(int dungeonLevel, const MonsterRegistry& monsters);
} // namespace MonsterFactory
