#include <cassert>
#include <optional>
#include <vector>

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

// One of the candidates, chosen evenly, or nullopt when there are none.
//
// Example:
//   pick_one({ Vector2D{ 4, 4 } }, ctx); // -> (4,4)
//   pick_one({}, ctx); // -> nullopt
std::optional<Vector2D> pick_one(const std::vector<Vector2D>& candidates, GameContext& ctx)
{
	if (candidates.empty())
	{
		return std::nullopt;
	}

	const int pick = ctx.dice->roll(0, static_cast<int>(candidates.size()) - 1);
	return candidates[static_cast<size_t>(pick)];
}
} // namespace

// A random floor tile anywhere inside the map's edge margin that a creature can be put on,
// or nullopt when the level has none. The candidates are enumerated and one is chosen, so
// a full or flooded level answers nothing instead of the search running forever.
std::optional<Vector2D> find_random_floor_tile(GameContext& ctx)
{
	assert(ctx.map);
	assert(ctx.dice);
	// can_place_at asks get_actor, which walks the creature list without checking it.
	assert(ctx.creatures);

	// Floor only, so a corridor or an open doorway is never a landing.
	std::vector<Vector2D> free{};
	for (int row = MAP_EDGE_MARGIN; row <= ctx.map->get_height() - MAP_EDGE_MARGIN; ++row)
	{
		for (int column = MAP_EDGE_MARGIN; column <= ctx.map->get_width() - MAP_EDGE_MARGIN; ++column)
		{
			const Vector2D here{ column, row };
			if (ctx.map->get_tile_type(here) == TileType::FLOOR && ctx.map->can_place_at(here, ctx))
			{
				free.push_back(here);
			}
		}
	}
	return pick_one(free, ctx);
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

	return pick_one(free_tiles_in(room, ctx), ctx);
}

} // namespace SpawnUtils
