#pragma once
// file: MonsterCreator.h
//
// Builds monsters from their parameters: rolls the ability scores and hit dice, arms
// them with their starting equipment, and gives them the AI their type names. The
// parameters come from the MonsterRegistry in ctx.

#include <memory>

#include "MonsterRegistry.h"

// Forward declarations
class Creature;
struct Vector2D;
struct GameContext;

namespace MonsterCreator
{
// A standard monster at pos, built from ctx.monsterRegistry's parameters for it. Not
// for class-based creatures, which their own classes construct.
[[nodiscard]] std::unique_ptr<Creature> create(Vector2D pos, MonsterId id, GameContext& ctx);

// A monster at pos built from params. An item it cannot carry in the slot the params
// name throws, naming the monster, the item and the slot.
[[nodiscard]] std::unique_ptr<Creature> create_from_params(Vector2D pos, const MonsterParams& params, GameContext& ctx);

} // namespace MonsterCreator
