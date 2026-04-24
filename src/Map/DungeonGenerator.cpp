// file: DungeonGenerator.cpp
#include <algorithm>
#include <limits>
#include <ranges>
#include <vector>

#include "../Actor/Item.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "../Tools/PrefabLibrary.h"
#include "../Utils/Vector2D.h"
#include "DungeonGenerator.h"
#include "DungeonRoom.h"
#include "Map.h"

// Cell dimensions: each grid cell is large enough to hold the largest room
// plus a guaranteed corridor gap on every side so rooms never share a wall.
// Defined here so both place_rooms and connect_all can reference them.
constexpr int CELL_W = ROOM_HORIZONTAL_MAX_SIZE + 8;
constexpr int CELL_H = ROOM_VERTICAL_MAX_SIZE + 8;
constexpr int LOOP_RADIUS = CELL_W * CELL_W + CELL_H * CELL_H;
constexpr int SKIP_PCTG = 20; // percent of cells left empty for variety

// ------------------------------------------------------------------- rooms

std::vector<DungeonRoom> DungeonGenerator::place_rooms(
	int mapWidth,
	int mapHeight,
	RandomDice& rng,
	const PrefabLibrary* prefabs) const
{
	const int gridCols = mapWidth / CELL_W;
	const int gridRows = mapHeight / CELL_H;

	std::vector<DungeonRoom> rooms;
	rooms.reserve(gridCols * gridRows);

	for (int gr = 0; gr < gridRows; ++gr)
	{
		for (int gc = 0; gc < gridCols; ++gc)
		{
			if (rng.roll(0, 99) < SKIP_PCTG)
			{
				continue;
			}

			const int cellX = gc * CELL_W;
			const int cellY = gr * CELL_H;

			// Prefab defines the room.  Pick a prefab that fits inside the cell
			// (total dims including wall border must not exceed CELL_W x CELL_H).
			// If no library is available (e.g. unit tests), fall back to random size.
			int roomWidth = rng.roll(ROOM_MIN_SIZE, ROOM_HORIZONTAL_MAX_SIZE);
			int roomHeight = rng.roll(ROOM_MIN_SIZE, ROOM_VERTICAL_MAX_SIZE);
			std::string chosenPrefab;

			if (prefabs && prefabs->count() > 0)
			{
				// Collect all prefabs that fit inside this cell.
				struct Candidate
				{
					int index;
					float weight;
				};
				std::vector<Candidate> candidates;
				const auto& all = prefabs->all();
				for (int i = 0; i < static_cast<int>(all.size()); ++i)
				{
					const Prefab& p = all[i];
					if (p.width() <= CELL_W && p.height() <= CELL_H)
					{
						candidates.push_back({ i, p.weight });
					}
				}

				if (!candidates.empty())
				{
					float total = 0.0f;
					for (const auto& c : candidates)
					{
						total += c.weight;
					}
					float roll = static_cast<float>(rng.roll(0, 999999)) / 1000000.0f * total;
					float accumulated = 0.0f;
					int chosen = candidates.back().index;
					for (const auto& c : candidates)
					{
						accumulated += c.weight;
						if (roll < accumulated)
						{
							chosen = c.index;
							break;
						}
					}
					const Prefab& p = all[chosen];
					roomWidth = p.width() - 2;
					roomHeight = p.height() - 2;
					chosenPrefab = p.name;
				}
			}

			// Random position inside the cell -- at least 1 tile from each edge.
			const int maxCol = cellX + CELL_W - roomWidth - 1;
			const int maxRow = cellY + CELL_H - roomHeight - 1;

			if (maxCol < cellX + 1 || maxRow < cellY + 1)
			{
				continue;
			}

			const int roomCol = rng.roll(cellX + 1, maxCol);
			const int roomRow = rng.roll(cellY + 1, maxRow);

			// Keep rooms inside map interior (wall ring must always exist).
			if (roomCol < 1 || roomRow < 1)
			{
				continue;
			}
			if (roomCol + roomWidth > mapWidth - 2)
			{
				continue;
			}
			if (roomRow + roomHeight > mapHeight - 2)
			{
				continue;
			}

			DungeonRoom room{ roomCol, roomRow, roomWidth, roomHeight };
			room.prefab_name = std::move(chosenPrefab);
			rooms.push_back(std::move(room));
		}
	}

	return rooms;
}

// --------------------------------------------------------------- corridors

void DungeonGenerator::connect_pair(
	std::vector<DungeonRoom>& rooms,
	int indexA,
	int indexB) const
{
	// Record bidirectional adjacency in the graph
	// (Actual corridor tile placement happens later in Map::place_from_graph)
	rooms[indexA].adjacentRoomIndices.push_back(indexB);
	rooms[indexB].adjacentRoomIndices.push_back(indexA);
}

void DungeonGenerator::connect_spanning_tree(
	std::vector<DungeonRoom>& rooms) const
{
	if (rooms.size() < 2)
	{
		return;
	}

	const int n = static_cast<int>(rooms.size());

	// ---- Prim's MST: guarantees full connectivity ----
	std::vector<bool> inTree(n, false);
	inTree[0] = true;
	int inCount = 1;

	while (inCount < n)
	{
		int bestA = -1;
		int bestB = -1;
		int bestDist = std::numeric_limits<int>::max();

		for (int i = 0; i < n; ++i)
		{
			if (!inTree[i])
			{
				continue;
			}

			for (int j = 0; j < n; ++j)
			{
				if (inTree[j])
				{
					continue;
				}

				const int dx = rooms[i].center_col() - rooms[j].center_col();
				const int dy = rooms[i].center_row() - rooms[j].center_row();
				const int dist = dx * dx + dy * dy;
				if (dist < bestDist)
				{
					bestDist = dist;
					bestA = i;
					bestB = j;
				}
			}
		}

		if (bestB < 0)
		{
			break;
		}

		connect_pair(rooms, bestA, bestB);
		inTree[bestB] = true;
		++inCount;
	}
}

void DungeonGenerator::add_extra_corridors(
	std::vector<DungeonRoom>& rooms,
	RandomDice& rng) const
{
	if (rooms.size() < 2)
	{
		return;
	}

	const int n = static_cast<int>(rooms.size());

	// ---- Extra short-range corridors (~33 %) to create loop paths ----
	// Limit to adjacent cells only (diagonal included). Rooms 2+ cells apart
	// produce corridors whose mid-turn segment can pass through an intermediate
	// room. CELL_W^2 + CELL_H^2 is the squared diagonal between adjacent cell
	// centres -- the tightest upper bound that still covers all adjacent pairs.
	//
	// Leaf rooms (degree-1 after the MST) are excluded from extra corridors.
	// Preserving them guarantees single-entrance rooms exist for treasure room
	// placement — connecting a leaf removes its only locked-door candidate.

	const int extraTarget = std::max(1, n / 3);
	int added = 0;
	const int maxAttempts = extraTarget * 6;

	for (int attempt = 0; attempt < maxAttempts && added < extraTarget; ++attempt)
	{
		const int a = rng.roll(0, n - 1);
		const int b = rng.roll(0, n - 1);
		if (a == b)
		{
			continue;
		}

		// Preserve leaf rooms — skip if either endpoint is currently degree-1.
		if (rooms[a].adjacentRoomIndices.size() == 1 ||
			rooms[b].adjacentRoomIndices.size() == 1)
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

		if (std::ranges::find(rooms[a].adjacentRoomIndices, b) != rooms[a].adjacentRoomIndices.end())
		{
			continue;
		}

		connect_pair(rooms, a, b);
		++added;
	}
}

// ----------------------------------------------------------------- generate

void DungeonGenerator::generate(
	int mapWidth,
	int mapHeight,
	RandomDice& rng,
	bool withActors,
	GameContext& ctx,
	Map& map) const
{
	std::vector<DungeonRoom> rooms = place_rooms(mapWidth, mapHeight, rng, ctx.prefabLibrary);

	if (rooms.empty())
	{
		return;
	}

	// Sort top-left first so room[0] is always the player spawn.
	std::ranges::sort(rooms,
		[](const DungeonRoom& a, const DungeonRoom& b)
		{ return (a.row * 10000 + a.col) < (b.row * 10000 + b.col); });

	// ---- Phase 1: Prim's MST — full connectivity, no cycles ----
	connect_spanning_tree(rooms);

	// Assign room types based on MST connectivity only.
	// Must run AFTER connect_spanning_tree but BEFORE add_extra_corridors
	// so extra loop edges do not demote leaf rooms to STANDARD.
	rooms[0].type = RoomType::ENTRANCE;
	for (size_t i = 1; i < rooms.size(); ++i)
	{
		const bool isLeaf = rooms[i].adjacentRoomIndices.size() == 1;
		if (isLeaf)
		{
			rooms[i].type = (rng.roll(1, 2) == 1) ? RoomType::DANGER : RoomType::TREASURE;
		}
		else
		{
			rooms[i].type = RoomType::STANDARD;
		}
	}

	// ---- Phase 2: extra corridors for loop paths ----
	// Does not affect room type assignment.
	add_extra_corridors(rooms, rng);

	// ---- Render graph to tiles and spawn content ----
	map.place_from_graph(rooms, withActors, ctx);

	if (ctx.rooms)
	{
		*ctx.rooms = rooms;
	}
}
