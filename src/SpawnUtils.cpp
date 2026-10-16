#include <cassert>
#include <optional>
#include <vector>

#include "Creature.h"
#include "DungeonRoom.h"
#include "GameContext.h"
#include "Map.h"
#include "RandomDice.h"
#include "SpawnUtils.h"
#include "Vector2D.h"

namespace SpawnUtils
{

namespace
{
constexpr int MAP_EDGE_MARGIN = 2;

// Every tile of the room a creature could be put on, in reading order.
//
// One pass over the room rather than a sampling loop, so an empty answer means the room
// is full rather than that a search gave up. A room is at most 14 by 9, which bounds
// this at 126 tests.
std::vector<Vector2D> free_tiles_in(const DungeonRoom& room, const GameContext& ctx)
{
	std::vector<Vector2D> free{};
	for (int row = room.row; row <= room.row_end(); ++row)
	{
		for (int column = room.col; column <= room.col_end(); ++column)
		{
			const Vector2D here{ column, row };
			// The tile is chosen before the monster is, so nothing here knows whether it swims.
			if (ctx.map->can_place_at(here, ctx))
			{
				free.push_back(here);
			}
		}
	}
	return free;
}
} // namespace

// Full-map random unoccupied floor tile.
// Skips the 1-tile border ring — map edges are always walls.
// Loops until a valid position is found; dungeons always have reachable floor.
Vector2D find_random_floor_tile(GameContext& ctx)
{
	assert(ctx.map);
	assert(ctx.dice);
	assert(ctx.creatures);
	assert(ctx.player());

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
		if (ctx.player()->position == pos)
		{
			return false;
		}
		return true;
	};

	while (true)
	{
		const Vector2D pos{
			ctx.dice->roll(MAP_EDGE_MARGIN, ctx.map->get_width() - MAP_EDGE_MARGIN),
			ctx.dice->roll(MAP_EDGE_MARGIN, ctx.map->get_height() - MAP_EDGE_MARGIN)
		};

		if (ctx.map->get_tile_type(pos) == TileType::FLOOR && is_position_free(pos))
		{
			return pos;
		}
	}
}

// A random tile of the room that a creature can be put on, or nullopt when the room has
// none left.
//
// The free tiles are enumerated and one is chosen, so nullopt is an answer rather than a
// search giving up, and there is no draw budget to pick. What makes a tile usable is
// Map::can_place_at, which is also what excludes water.
std::optional<Vector2D> find_random_room_position(const DungeonRoom& room, GameContext& ctx)
{
	assert(ctx.map);
	assert(ctx.dice);

	const std::vector<Vector2D> free = free_tiles_in(room, ctx);
	if (free.empty())
	{
		return std::nullopt;
	}

	const int pick = ctx.dice->roll(0, static_cast<int>(free.size()) - 1);
	return free[static_cast<size_t>(pick)];
}

} // namespace SpawnUtils
