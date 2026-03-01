// file: DungeonGenerator.cpp
#include <algorithm>
#include <limits>
#include <ranges>

#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "DungeonGenerator.h"
#include "Map.h"

// ------------------------------------------------------------------ helpers

static Vector2D v(int col, int row) noexcept
{
	return Vector2D{ col, row };
}

// ------------------------------------------------------------------- rooms

std::vector<DungeonRoom> DungeonGenerator::place_rooms(
	int map_width,
	int map_height,
	RandomDice& rng) const
{
	// Each cell is wide/tall enough to hold the largest room plus a corridor
	// gap on every side so rooms never share a wall tile.
	constexpr int CELL_W = ROOM_HORIZONTAL_MAX_SIZE + 3;
	constexpr int CELL_H = ROOM_VERTICAL_MAX_SIZE + 4;
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
				continue;

			const int cell_x = gc * CELL_W;
			const int cell_y = gr * CELL_H;

			const int room_w = rng.roll(ROOM_MIN_SIZE, ROOM_HORIZONTAL_MAX_SIZE);
			const int room_h = rng.roll(ROOM_MIN_SIZE, ROOM_VERTICAL_MAX_SIZE);

			// Random position inside the cell -- at least 1 tile from each edge.
			const int max_col = cell_x + CELL_W - room_w - 1;
			const int max_row = cell_y + CELL_H - room_h - 1;

			if (max_col < cell_x + 1 || max_row < cell_y + 1)
				continue;

			const int room_col = rng.roll(cell_x + 1, max_col);
			const int room_row = rng.roll(cell_y + 1, max_row);

			// Keep rooms inside map interior (wall ring must always exist).
			if (room_col < 1 || room_row < 1)
				continue;
			if (room_col + room_w > map_width - 2)
				continue;
			if (room_row + room_h > map_height - 2)
				continue;

			rooms.push_back({ room_col, room_row, room_w, room_h });
		}
	}

	return rooms;
}

// --------------------------------------------------------------- corridors

void DungeonGenerator::connect_pair(
	const DungeonRoom& a,
	const DungeonRoom& b,
	Map& map) const
{
	const bool a_below = a.top_wall() > b.bottom_wall();
	const bool a_right = a.left_wall() > b.right_wall();
	const bool a_above = a.bottom_wall() < b.top_wall();
	const bool a_left = a.right_wall() < b.left_wall();

	if (a_below)
	{
		// a is below b -- corridor rises from a, crosses horizontally, drops to b
		const int mid = (b.bottom_wall() + a.top_wall()) / 2;
		map.dig_corridor(v(a.center_col(), a.top_wall()), v(a.center_col(), mid));
		map.dig_corridor(v(a.center_col(), mid), v(b.center_col(), mid));
		map.dig_corridor(v(b.center_col(), mid), v(b.center_col(), b.bottom_wall()));
	}
	else if (a_right)
	{
		// a is to the right of b
		const int mid = (b.right_wall() + a.left_wall()) / 2;
		map.dig_corridor(v(a.left_wall(), a.center_row()), v(mid, a.center_row()));
		map.dig_corridor(v(mid, a.center_row()), v(mid, b.center_row()));
		map.dig_corridor(v(mid, b.center_row()), v(b.right_wall(), b.center_row()));
	}
	else if (a_above)
	{
		// a is above b
		const int mid = (a.bottom_wall() + b.top_wall()) / 2;
		map.dig_corridor(v(a.center_col(), a.bottom_wall()), v(a.center_col(), mid));
		map.dig_corridor(v(a.center_col(), mid), v(b.center_col(), mid));
		map.dig_corridor(v(b.center_col(), mid), v(b.center_col(), b.top_wall()));
	}
	else if (a_left)
	{
		// a is to the left of b
		const int mid = (a.right_wall() + b.left_wall()) / 2;
		map.dig_corridor(v(a.right_wall(), a.center_row()), v(mid, a.center_row()));
		map.dig_corridor(v(mid, a.center_row()), v(mid, b.center_row()));
		map.dig_corridor(v(mid, b.center_row()), v(b.left_wall(), b.center_row()));
	}
	else
	{
		// Rooms are adjacent or overlapping -- direct L-shape between centres.
		map.dig_corridor(
			v(a.center_col(), a.center_row()),
			v(b.center_col(), b.center_row()));
	}
}

void DungeonGenerator::connect_all(
	const std::vector<DungeonRoom>& rooms,
	RandomDice& rng,
	Map& map) const
{
	if (rooms.size() < 2)
		return;

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
				continue;
			for (int j = 0; j < n; ++j)
			{
				if (in_tree[j])
					continue;
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
			break;

		connect_pair(rooms[best_a], rooms[best_b], map);
		in_tree[best_b] = true;
		++in_count;
	}

	// ---- Extra short-range corridors (~20 %) to create loop paths ----
	constexpr int LOOP_RADIUS = 35 * 35; // max squared distance for a loop edge
	const int extra_target = std::max(1, n / 5);
	int added = 0;
	const int max_attempts = extra_target * 6;

	for (int attempt = 0; attempt < max_attempts && added < extra_target; ++attempt)
	{
		const int a = rng.roll(0, n - 1);
		const int b = rng.roll(0, n - 1);
		if (a == b)
			continue;

		const int dx = rooms[a].center_col() - rooms[b].center_col();
		const int dy = rooms[a].center_row() - rooms[b].center_row();
		const int dist = dx * dx + dy * dy;
		if (dist > LOOP_RADIUS)
			continue;

		connect_pair(rooms[a], rooms[b], map);
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
		return;

	// Sort top-left first so room[0] is always the player spawn.
	std::ranges::sort(rooms, [](const DungeonRoom& a, const DungeonRoom& b)
		{ return (a.row * 10000 + a.col) < (b.row * 10000 + b.col); });

	for (int i = 0; i < static_cast<int>(rooms.size()); ++i)
		map.create_room(rooms[i], i == 0, withActors, ctx);

	connect_all(rooms, rng, map);
}
