// file: DungeonGenerator.cpp
#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <vector>

#include "DungeonGenerator.h"
#include "DungeonRoom.h"
#include "GameContext.h"
#include "Map.h"
#include "PrefabLibrary.h"
#include "RandomDice.h"

// Cell dimensions: each grid cell is large enough to hold the largest room
// plus a guaranteed corridor gap on every side so rooms never share a wall.
// Defined here so both place_rooms and connect_all can reference them.
constexpr int CELL_W = ROOM_HORIZONTAL_MAX_SIZE + 8;
constexpr int CELL_H = ROOM_VERTICAL_MAX_SIZE + 8;
constexpr int LOOP_RADIUS = CELL_W * CELL_W + CELL_H * CELL_H;
constexpr int SKIP_PCTG = 20; // percent of cells left empty for variety

// ---------------------------------------------------------------------------
// Shape selection
//
// Weights must sum to 100. Shapes with dimension requirements fall back to
// RECT when the generated room is too small to look meaningful.
// ---------------------------------------------------------------------------

namespace
{

RoomShape pick_room_shape(int width, int height, RandomDice& rng)
{
	// Weighted table: { shape, weight, minWidth, minHeight }
	struct ShapeEntry
	{
		RoomShape shape;
		int weight;
		int minWidth;
		int minHeight;
	};

	constexpr std::array<ShapeEntry, 5> table{
		ShapeEntry{ RoomShape::RECT, 40, 0, 0 },
		ShapeEntry{ RoomShape::CHAMFERED, 25, 6, 6 },
		ShapeEntry{ RoomShape::L_SHAPE, 20, 9, 7 },
		ShapeEntry{ RoomShape::PILLARED, 10, 8, 6 },
		ShapeEntry{ RoomShape::CROSS, 5, 10, 7 },
	};

	// Build eligible entries for this room size.
	int totalWeight = 0;
	for (const auto& entry : table)
	{
		if (width >= entry.minWidth && height >= entry.minHeight)
		{
			totalWeight += entry.weight;
		}
	}

	if (totalWeight <= 0)
	{
		return RoomShape::RECT;
	}

	int roll = rng.roll(1, totalWeight);
	int accumulated = 0;
	for (const auto& entry : table)
	{
		if (width < entry.minWidth || height < entry.minHeight)
		{
			continue;
		}
		accumulated += entry.weight;
		if (roll <= accumulated)
		{
			return entry.shape;
		}
	}

	return RoomShape::RECT;
}

} // namespace

// ------------------------------------------------------------------- rooms

std::vector<DungeonRoom> DungeonGenerator::place_rooms(
	int mapWidth,
	int mapHeight,
	RandomDice& rng,
	const PrefabLibrary* prefabs) const
{
	assert(
		prefabs &&
		"DungeonGenerator::place_rooms: called without a prefab library");
	const int gridCols = mapWidth / CELL_W;
	const int gridRows = mapHeight / CELL_H;

	std::vector<DungeonRoom> rooms;
	rooms.reserve(gridCols * gridRows);

	for (int gridRow = 0; gridRow < gridRows; ++gridRow)
	{
		for (int gridCol = 0; gridCol < gridCols; ++gridCol)
		{
			if (rng.roll(0, 99) < SKIP_PCTG)
			{
				continue;
			}

			const int cellX = gridCol * CELL_W;
			const int cellY = gridRow * CELL_H;

			// Pick a prefab that fits the cell, wall border included; with no library, as in unit tests, a random size.
			int roomWidth = rng.roll(ROOM_MIN_SIZE, ROOM_HORIZONTAL_MAX_SIZE);
			int roomHeight = rng.roll(ROOM_MIN_SIZE, ROOM_VERTICAL_MAX_SIZE);
			std::string chosenPrefab;

			if (prefabs->count() > 0)
			{
				// Collect all prefabs that fit inside this cell.
				struct Candidate
				{
					int index;
					float weight;
				};

				std::vector<Candidate> candidates;
				const auto& all = prefabs->all();
				for (
					int prefabIndex = 0;
					prefabIndex < static_cast<int>(all.size());
					++prefabIndex)
				{
					const Prefab& prefab = all[prefabIndex];
					if (prefab.width() <= CELL_W && prefab.height() <= CELL_H)
					{
						candidates.push_back({ prefabIndex, prefab.weight });
					}
				}

				if (!candidates.empty())
				{
					float total = 0.0f;
					for (const auto& candidate : candidates)
					{
						total += candidate.weight;
					}
					float roll = static_cast<float>(rng.roll(0, 999999)) /
						1000000.0f * total;
					float accumulated = 0.0f;
					int chosen = candidates.back().index;
					for (const auto& candidate : candidates)
					{
						accumulated += candidate.weight;
						if (roll < accumulated)
						{
							chosen = candidate.index;
							break;
						}
					}
					const Prefab& prefab = all[chosen];
					roomWidth = prefab.width() - 2;
					roomHeight = prefab.height() - 2;
					chosenPrefab = prefab.name;
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
			room.prefabName = std::move(chosenPrefab);

			// Prefab and shape vary independently, and the pair can split a room;
			// Map::create_room checks the floor and drops the shape when it does.
			room.shape = pick_room_shape(roomWidth, roomHeight, rng);
			room.shapeVariant = rng.roll(0, 3);

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

	const int roomCount = static_cast<int>(rooms.size());

	// ---- Prim's MST: guarantees full connectivity ----
	std::vector<bool> inTree(roomCount, false);
	inTree[0] = true;
	int inCount = 1;

	while (inCount < roomCount)
	{
		int bestA = -1;
		int bestB = -1;
		int bestDist = std::numeric_limits<int>::max();

		for (int treeRoom = 0; treeRoom < roomCount; ++treeRoom)
		{
			if (!inTree[treeRoom])
			{
				continue;
			}

			for (int outsideRoom = 0; outsideRoom < roomCount; ++outsideRoom)
			{
				if (inTree[outsideRoom])
				{
					continue;
				}

				const int deltaCol = rooms[treeRoom].center_col() -
					rooms[outsideRoom].center_col();
				const int deltaRow = rooms[treeRoom].center_row() -
					rooms[outsideRoom].center_row();
				const int dist = deltaCol * deltaCol + deltaRow * deltaRow;
				if (dist < bestDist)
				{
					bestDist = dist;
					bestA = treeRoom;
					bestB = outsideRoom;
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

	const int roomCount = static_cast<int>(rooms.size());

	// ---- Extra short-range corridors (~33 %) to create loop paths ----
	// Adjacent cells only, so no corridor crosses a third room; leaf rooms keep one entrance for vaults.

	const int extraTarget = std::max(1, roomCount / 3);
	int added = 0;
	const int maxAttempts = extraTarget * 6;

	for (
		int attempt = 0; attempt < maxAttempts && added < extraTarget;
		++attempt)
	{
		const int fromRoom = rng.roll(0, roomCount - 1);
		const int toRoom = rng.roll(0, roomCount - 1);
		if (fromRoom == toRoom)
		{
			continue;
		}

		// Preserve leaf rooms - skip if either endpoint is currently degree-1.
		if (rooms[fromRoom].adjacentRoomIndices.size() == 1 ||
			rooms[toRoom].adjacentRoomIndices.size() == 1)
		{
			continue;
		}

		const int deltaCol =
			rooms[fromRoom].center_col() - rooms[toRoom].center_col();
		const int deltaRow =
			rooms[fromRoom].center_row() - rooms[toRoom].center_row();
		const int dist = deltaCol * deltaCol + deltaRow * deltaRow;
		if (dist > LOOP_RADIUS)
		{
			continue;
		}

		if (std::ranges::contains(rooms[fromRoom].adjacentRoomIndices, toRoom))
		{
			continue;
		}

		connect_pair(rooms, fromRoom, toRoom);
		++added;
	}
}

// ----------------------------------------------------------------- generate

void DungeonGenerator::generate(
	int mapWidth,
	int mapHeight,
	RandomDice& rng,
	GameContext& ctx,
	Map& map) const
{
	std::vector<DungeonRoom> rooms =
		place_rooms(mapWidth, mapHeight, rng, ctx.prefabLibrary);

	if (rooms.empty())
	{
		return;
	}

	// Sort top-left first so room[0] is always the player spawn.
	std::ranges::sort(
		rooms,
		[](const DungeonRoom& left, const DungeonRoom& right)
		{
			return (left.row * 10000 + left.col) <
				(right.row * 10000 + right.col);
		});

	// ---- Phase 1: Prim's MST - full connectivity, no cycles ----
	connect_spanning_tree(rooms);

	// From MST connectivity only: after connect_spanning_tree and before add_extra_corridors,
	// so loop edges do not demote leaf rooms.
	rooms[0].type = RoomType::ENTRANCE;
	for (size_t roomIndex = 1; roomIndex < rooms.size(); ++roomIndex)
	{
		const bool isLeaf = rooms[roomIndex].adjacentRoomIndices.size() == 1;
		if (isLeaf)
		{
			rooms[roomIndex].type =
				(rng.roll(1, 2) == 1) ? RoomType::DANGER : RoomType::TREASURE;
		}
		else
		{
			rooms[roomIndex].type = RoomType::STANDARD;
		}
	}

	// ---- Phase 2: extra corridors for loop paths ----
	// Does not affect room type assignment.
	add_extra_corridors(rooms, rng);

	// ---- Render graph to tiles and spawn content ----
	map.place_from_graph(rooms, ctx);

	assert(ctx.rooms && "DungeonGenerator::generate: called without rooms");
	*ctx.rooms = rooms;
}
