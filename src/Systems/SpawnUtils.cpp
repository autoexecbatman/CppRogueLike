#include <cassert>
#include <optional>

#include "../Actor/Creature.h"
#include "../Core/GameContext.h"
#include "../Map/DungeonRoom.h"
#include "../Map/Map.h"
#include "../Random/RandomDice.h"
#include "../Utils/Vector2D.h"
#include "SpawnUtils.h"

namespace SpawnUtils
{

namespace
{
    constexpr int MAP_EDGE_MARGIN = 2;
}

// Full-map random unoccupied floor tile.
// Skips the 1-tile border ring — map edges are always walls.
// Loops until a valid position is found; dungeons always have reachable floor.
Vector2D find_random_floor_tile(GameContext& ctx)
{
    assert(ctx.map);
    assert(ctx.dice);
    assert(ctx.creatures);
    assert(ctx.player);

    auto is_position_free = [&](Vector2D pos) -> bool
    {
        for (const auto& creature : *ctx.creatures)
        {
            assert(creature);
            if (creature->position == pos)
            {
                return false;
            }
        }
        if (ctx.player->position == pos)
        {
            return false;
        }
        return true;
    };

    while (true)
    {
        const Vector2D pos
        {
            ctx.dice->roll(MAP_EDGE_MARGIN, ctx.map->get_width() - MAP_EDGE_MARGIN),
            ctx.dice->roll(MAP_EDGE_MARGIN, ctx.map->get_height() - MAP_EDGE_MARGIN)
        };

        if (ctx.map->get_tile_type(pos) == TileType::FLOOR && is_position_free(pos))
        {
            return pos;
        }
    }
}

// Room-scoped random walkable, unoccupied, decoration-free tile.
// Returns nullopt when no valid position found within ROOM_MAX_TRIES — room may be full.
std::optional<Vector2D> find_random_room_position(const DungeonRoom& room, GameContext& ctx)
{
    assert(ctx.map);
    assert(ctx.dice);

    while(true)
    {
        const Vector2D pos
        {
            ctx.dice->roll(room.col, room.col_end()),
            ctx.dice->roll(room.row, room.row_end())
        };

        if (ctx.map->can_walk(pos, ctx) &&
            ctx.map->get_actor(pos, ctx) == nullptr &&
            ctx.map->find_decoration_at(pos, ctx) == nullptr)
        {
            return pos;
        }
    }

    return std::nullopt;
}

} // namespace SpawnUtils
