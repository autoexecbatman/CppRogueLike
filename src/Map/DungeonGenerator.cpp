// file: DungeonGenerator.cpp
#include <algorithm>
#include <limits>
#include <vector>

#include "../Actor/Item.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "../Utils/Vector2D.h"
#include "DungeonGenerator.h"
#include "DungeonRoom.h"
#include "Map.h"

// ------------------------------------------------------------------ helpers

static Vector2D v(int col, int row) noexcept
{
	return Vector2D{ col, row };
}

// Cell dimensions: each grid cell is large enough to hold the largest room
// plus a guaranteed corridor gap on every side so rooms never share a wall.
// Defined here so both place_rooms and connect_all can reference them.
static constexpr int CELL_W = ROOM_HORIZONTAL_MAX_SIZE + 3;
static constexpr int CELL_H = ROOM_VERTICAL_MAX_SIZE + 4;

// ------------------------------------------------------------------- rooms

std::vector<DungeonRoom> DungeonGenerator::place_rooms(
	int map_width,
	int map_height,
	RandomDice& rng) const
{
	constexpr int SKIP_PCTG = 20; // percent of cells left empty for variety

	const int grid_cols = map_width / CELL_W;
	const int grid_rows = map_height / CELL_H;

	std::vector<DungeonRoom> rooms;
	rooms.reserve(grid_cols * grid_rows);

	for (int gr = 0; gr < grid_rows; ++gr)
	{
		for (int gc = 0; gc < grid_cols; ++gc)
		{
			if (rng.roll(0, 99) < SKIP_PCTG)
			{
				continue;
			}

			const int cell_x = gc * CELL_W;
			const int cell_y = gr * CELL_H;

			const int room_w = rng.roll(ROOM_MIN_SIZE, ROOM_HORIZONTAL_MAX_SIZE);
			const int room_h = rng.roll(ROOM_MIN_SIZE, ROOM_VERTICAL_MAX_SIZE);

			// Random position inside the cell -- at least 1 tile from each edge.
			const int max_col = cell_x + CELL_W - room_w - 1;
			const int max_row = cell_y + CELL_H - room_h - 1;

			if (max_col < cell_x + 1 || max_row < cell_y + 1)
			{
				continue;
			}

			const int room_col = rng.roll(cell_x + 1, max_col);
			const int room_row = rng.roll(cell_y + 1, max_row);

			// Keep rooms inside map interior (wall ring must always exist).
			if (room_col < 1 || room_row < 1)
			{
				continue;
			}
			if (room_col + room_w > map_width - 2)
			{
				continue;
			}
			if (room_row + room_h > map_height - 2)
			{
				continue;
			}

			rooms.push_back({ room_col, room_row, room_w, room_h });
		}
	}

	return rooms;
}

// --------------------------------------------------------------- corridors

void DungeonGenerator::connect_pair(
	std::vector<DungeonRoom>& rooms,
	int a_index,
	int b_index) const
{
	// Record bidirectional adjacency in the graph
	// (Actual corridor tile placement happens later in Map::place_from_graph)
	rooms[a_index].adjacent_room_indices.push_back(b_index);
	rooms[b_index].adjacent_room_indices.push_back(a_index);
}

void DungeonGenerator::connect_all(
	std::vector<DungeonRoom>& rooms,
	RandomDice& rng) const
{
	if (rooms.size() < 2)
	{
		return;
	}

	const int n = static_cast<int>(rooms.size());

	// ---- Prim's MST: guarantees full connectivity ----
	std::vector<bool> in_tree(n, false);
	in_tree[0] = true;
	int in_count = 1;

	while (in_count < n)
	{
		int best_a = -1;
		int best_b = -1;
		int best_dist = std::numeric_limits<int>::max();

		for (int i = 0; i < n; ++i)
		{
			if (!in_tree[i])
			{
				continue;
			}

			for (int j = 0; j < n; ++j)
			{
				if (in_tree[j])
				{
					continue;
				}

				const int dx = rooms[i].center_col() - rooms[j].center_col();
				const int dy = rooms[i].center_row() - rooms[j].center_row();
				const int dist = dx * dx + dy * dy;
				if (dist < best_dist)
				{
					best_dist = dist;
					best_a = i;
					best_b = j;
				}
			}
		}

		if (best_b < 0)
		{
			break;
		}

		connect_pair(rooms, best_a, best_b);
		in_tree[best_b] = true;
		++in_count;
	}

	// ---- Extra short-range corridors (~20 %) to create loop paths ----
	// Limit to adjacent cells only (diagonal included). Rooms 2+ cells apart
	// produce corridors whose mid-turn segment can pass through an intermediate
	// room.  CELL_W^2 + CELL_H^2 is the squared diagonal between adjacent cell
	// centres -- the tightest upper bound that still covers all adjacent pairs.
	constexpr int LOOP_RADIUS = CELL_W * CELL_W + CELL_H * CELL_H;
	const int extra_target = std::max(1, n / 5);
	int added = 0;
	const int max_attempts = extra_target * 6;

	for (int attempt = 0; attempt < max_attempts && added < extra_target; ++attempt)
	{
		const int a = rng.roll(0, n - 1);
		const int b = rng.roll(0, n - 1);
		if (a == b)
		{
			continue;
		}

		const int dx = rooms[a].center_col() - rooms[b].center_col();
		const int dy = rooms[a].center_row() - rooms[b].center_row();
		const int dist = dx * dx + dy * dy;
		if (dist > LOOP_RADIUS)
		{
			continue;
		}

		connect_pair(rooms, a, b);
		++added;
	}
}

// ----------------------------------------------------------------- generate

void DungeonGenerator::generate(
	int map_width,
	int map_height,
	RandomDice& rng,
	bool withActors,
	GameContext& ctx,
	Map& map) const
{
	std::vector<DungeonRoom> rooms = place_rooms(map_width, map_height, rng);

	if (rooms.empty())
	{
		return;
	}

	// Sort top-left first so room[0] is always the player spawn.
	std::ranges::sort(rooms,
		[](const DungeonRoom& a, const DungeonRoom& b)
		{ return (a.row * 10000 + a.col) < (b.row * 10000 + b.col); });

	// Assign room types — entrance is always room[0], last room is danger
	rooms[0].type = RoomType::ENTRANCE;
	rooms.back().type = RoomType::DANGER;
	for (size_t i = 1; i + 1 < rooms.size(); ++i)
	{
		rooms[i].type = (rng.roll(1, 100) <= 20) ? RoomType::DANGER : RoomType::CORRIDOR;
	}

	// ---- Build pure graph (rooms + edges) ----
	connect_all(rooms, rng);

	// ---- Render graph to tiles and spawn content ----
	map.place_from_graph(rooms, withActors, ctx);

	if (ctx.rooms)
	{
		*ctx.rooms = rooms;
	}
}
