#pragma once

#include <optional>

#include "../Map/DungeonRoom.h"
#include "../Utils/Vector2D.h"

struct GameContext;

// Centralized spatial placement utilities.
// All "find a free tile" operations route through here — eliminates duplicated
// random-sampling logic scattered across Pickable, SpellSystem, CreatureManager, etc.
namespace SpawnUtils
{
    // Full-map random unoccupied floor tile.
    // Loops until a valid position is found — dungeons always have reachable floor.
    Vector2D find_random_floor_tile(GameContext& ctx);

    // Room-scoped random walkable, unoccupied, decoration-free tile.
    // Returns nullopt when no valid position found within MAX_TRIES — room may be full.
    std::optional<Vector2D> find_random_room_position(const DungeonRoom& room, GameContext& ctx);
}
