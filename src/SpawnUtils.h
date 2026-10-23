#pragma once

#include <optional>

#include "DungeonRoom.h"
#include "Vector2D.h"

struct GameContext;

// Centralized spatial placement utilities.
// All "find a free tile" operations route through here - eliminates duplicated
// random-sampling logic scattered across Pickable, SpellSystem, CreatureManager, etc.
namespace SpawnUtils
{
// A random floor tile inside the map's margin that a creature can be put on, or nullopt
// when the level has none; the tiles are enumerated, so a full level answers nothing.
std::optional<Vector2D> find_random_floor_tile(GameContext& ctx);

// A random tile of the room a creature can be put on: walkable, unoccupied, free of
// decoration and dry. Answers nullopt when the room has none, which is exact rather
// than a search giving up.
std::optional<Vector2D> find_random_room_position(
	const DungeonRoom& room,
	GameContext& ctx);
} // namespace SpawnUtils
