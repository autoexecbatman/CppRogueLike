// file: Map/TreasureRoom.cpp
#include <algorithm>
#include <cassert>
#include <cmath>
#include <format>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "Creature.h"
#include "InventoryOperations.h"
#include "GameContext.h"
#include "ItemCreator.h"
#include "MonsterCreator.h"
#include "RandomDice.h"
#include "LevelManager.h"
#include "MessageSystem.h"
#include "Vector2D.h"
#include "DungeonNames.h"
#include "DungeonRoom.h"
#include "Map.h"
#include "TileType.h"
#include "TreasureRoom.h"

namespace
{
	// Whether `target` can be reached from `start` without crossing a locked
	// door or a wall. A jailer must spawn on the corridor side of the door it
	// guards, so a spawn that fails this is in a pocket of its own.
	bool is_reachable_without_locked_doors(
		Vector2D start,
		Vector2D target,
		const Map& map)
	{
		if (start == target)
		{
			return true;
		}

		// Use a flat visited array sized to the known map extents.
		const int width = map.get_width();
		const int height = map.get_height();
		std::vector<bool> visited(static_cast<size_t>(width) * height, false);

		auto mark = [&](Vector2D pos)
		{
			visited[static_cast<size_t>(pos.y) * width + pos.x] = true;
		};
		auto was_visited = [&](Vector2D pos) -> bool
		{
			return visited[static_cast<size_t>(pos.y) * width + pos.x];
		};
		auto can_traverse = [&](Vector2D pos) -> bool
		{
			if (!map.is_in_bounds(pos)) return false;
			if (map.is_door_locked(pos)) return false;
			return map.get_tile_type(pos) != TileType::WALL;
		};

		std::queue<Vector2D> frontier;
		frontier.push(start);
		mark(start);

		while (!frontier.empty())
		{
			const Vector2D current = frontier.front();
			frontier.pop();

			if (current == target)
			{
				return true;
			}

			for (Vector2D dir : { DIR_N, DIR_S, DIR_E, DIR_W, DIR_NE, DIR_NW, DIR_SE, DIR_SW })
			{
				const Vector2D next = current + dir;
				if (!was_visited(next) && can_traverse(next))
				{
					mark(next);
					frontier.push(next);
				}
			}
		}

		return false;
	}
}

int TreasureRoom::count_entrances(const Map& map, const DungeonRoom& room)
{
	int count = 0;
	for (int y = room.top_wall(); y <= room.bottom_wall(); ++y)
	{
		for (int x = room.left_wall(); x <= room.right_wall(); ++x)
		{
			const Vector2D pos{ x, y };
			if (!map.is_in_bounds(pos) || room.contains(x, y))
			{
				continue;
			}
			if (map.get_tile_type(pos) != TileType::CLOSED_DOOR)
			{
				continue;
			}
			for (Vector2D dir : { DIR_N, DIR_S, DIR_E, DIR_W })
			{
				if (room.contains((pos + dir).x, (pos + dir).y))
				{
					++count;
					break;
				}
			}
		}
	}
	return count;
}

void TreasureRoom::setup_guard(const DungeonRoom& room, GameContext& ctx)
{
	assert(ctx.map && "TreasureRoom::setup_guard called without a map");
	assert(ctx.contentRegistry && "TreasureRoom::setup_guard called without contentRegistry");
	assert(ctx.creatures && "TreasureRoom::setup_guard called without creatures");
	assert(ctx.dice && "TreasureRoom::setup_guard called without dice");
	assert(ctx.player() && "TreasureRoom::setup_guard called without player");

	// Two separate passes over the wall border:
	//
	// Pass 1 — lock EVERY genuine room entrance.
	//   A door belongs to this room iff one of its cardinal neighbors is inside
	//   room.contains(). Lock it regardless of what tile type is outside.
	//
	// Pass 2 — collect jailer spawn candidates.
	//   Walk outward from each locked entrance up to MAX_WALK steps.
	//   Break only on walls; skip actors/decorations/other doors.
	//   Reachability filter (below) guarantees the spawn is on the corridor side.

	auto find_inward_dir = [&](Vector2D doorPos) -> Vector2D
	{
		for (Vector2D dir : { DIR_N, DIR_S, DIR_E, DIR_W })
		{
			if (room.contains((doorPos + dir).x, (doorPos + dir).y))
			{
				return dir;
			}
		}
		return { 0, 0 }; // no room-interior neighbor found
	};

	// Pass 1: lock all genuine entrances.
	for (int y = room.top_wall(); y <= room.bottom_wall(); ++y)
	{
		for (int x = room.left_wall(); x <= room.right_wall(); ++x)
		{
			Vector2D doorPos{ x, y };
			if (!ctx.map->is_in_bounds(doorPos) || room.contains(x, y))
			{
				continue;
			}
			if (ctx.map->get_tile_type(doorPos) != TileType::CLOSED_DOOR)
			{
				continue;
			}
			const Vector2D inward = find_inward_dir(doorPos);
			if (inward.x == 0 && inward.y == 0)
			{
				continue;
			}
			ctx.map->tiles[ctx.map->get_index(doorPos)].doorState = DoorState::CLOSED_LOCKED;
		}
	}

	// Pass 2: collect jailer spawn candidates.
	// For each locked entrance, walk outward (away from the room) up to MAX_WALK
	// steps. Break only on map boundary or solid wall — skip past actors, decorations,
	// and other doors (temporary occupants that do not block the corridor permanently).
	struct DoorCandidate
	{
		Vector2D doorPos;
		Vector2D spawnPos;
	};

	std::vector<DoorCandidate> candidates;

	for (int y = room.top_wall(); y <= room.bottom_wall(); ++y)
	{
		for (int x = room.left_wall(); x <= room.right_wall(); ++x)
		{
			Vector2D doorPos{ x, y };
			if (!ctx.map->is_in_bounds(doorPos) || room.contains(x, y))
			{
				continue;
			}
			if (!ctx.map->is_door_locked(doorPos))
			{
				continue;
			}

			const Vector2D inward = find_inward_dir(doorPos);
			if (inward.x == 0 && inward.y == 0)
			{
				continue;
			}

			std::optional<Vector2D> spawnPos;
			constexpr int MAX_WALK = 5;
			for (int step = 1; step <= MAX_WALK; ++step)
			{
				const Vector2D candidate = doorPos - (inward * step);
				if (!ctx.map->is_in_bounds(candidate))
				{
					break; // left the map
				}
				const TileType tileType = ctx.map->get_tile_type(candidate);
				if (tileType == TileType::WALL)
				{
					break; // solid obstruction — corridor ends here
				}
				// Skip temporarily-occupied or non-floor tiles; keep stepping.
				if (tileType == TileType::CLOSED_DOOR)
				{
					continue;
				}
				if (ctx.map->get_actor(candidate, ctx) != nullptr)
				{
					continue;
				}
				if (ctx.map->find_decoration_at(candidate, ctx) != nullptr)
				{
					continue;
				}
				spawnPos = candidate;
				break;
			}

			if (!spawnPos)
			{
				continue;
			}

			candidates.push_back({ doorPos, *spawnPos });
		}
	}

	// Helper: undo all locks Pass 1 applied so the map is left consistent
	// when we bail out without placing a jailer.
	auto unlock_all_room_entrances = [&]()
	{
		for (int y = room.top_wall(); y <= room.bottom_wall(); ++y)
		{
			for (int x = room.left_wall(); x <= room.right_wall(); ++x)
			{
				const Vector2D pos{ x, y };
				if (!ctx.map->is_in_bounds(pos) || room.contains(x, y))
				{
					continue;
				}
				if (ctx.map->is_door_locked(pos))
				{
					ctx.map->tiles[ctx.map->get_index(pos)].doorState = DoorState::CLOSED_UNLOCKED;
				}
			}
		}
	};

	if (candidates.empty())
	{
		unlock_all_room_entrances();
		return;
	}

	// With all doors now locked, filter to candidates whose spawn tile is
	// still reachable from the stairs. A dead-end corridor that only connects
	// back through a locked door will fail this check.
	auto manhattan = [](Vector2D a, Vector2D b)
	{
		return std::abs(a.x - b.x) + std::abs(a.y - b.y);
	};

	const Vector2D playerPos = ctx.player()->position;
	const bool stairsAvailable = ctx.stairs != nullptr;

	const DoorCandidate* best = nullptr;
	int bestDist = std::numeric_limits<int>::max();

	for (const DoorCandidate& c : candidates)
	{
		if (stairsAvailable &&
			!is_reachable_without_locked_doors(c.spawnPos, ctx.stairs->position, *ctx.map))
		{
			continue; // dead-end pocket — skip
		}

		const int dist = manhattan(c.spawnPos, playerPos);
		if (dist < bestDist)
		{
			bestDist = dist;
			best = &c;
		}
	}

	if (best == nullptr)
	{
		// No corridor-side spawn is reachable from the stairs.
		// Unlock all entrances so the room is not permanently inaccessible.
		unlock_all_room_entrances();
		return;
	}

	assert(
		(ctx.stairs == nullptr ||
		is_reachable_without_locked_doors(best->spawnPos, ctx.stairs->position, *ctx.map)) &&
		"TreasureRoom::setup_guard: reachability filter passed but assert disagrees");

	auto jailer = MonsterCreator::create_from_params(
		best->spawnPos,
		MonsterCreator::get_params("dungeon_jailer"),
		ctx);

	auto key = ItemCreator::create("dungeon_key", best->spawnPos, *ctx.contentRegistry);
	[[maybe_unused]] const auto giveKeyToJailerResult = InventoryOperations::add_item_to_inventory(jailer->inventoryData, std::move(key), *jailer);
	assert(giveKeyToJailerResult.has_value());

	ctx.creatures->push_back(std::move(jailer));
}

void TreasureRoom::create(
	const DungeonRoom& room,
	int quality,
	RandomDice& generationRng,
	GameContext& ctx)
{
	assert(ctx.map && "TreasureRoom::create called without a map");
	assert(ctx.levelManager && "TreasureRoom::create called without levelManager");
	assert(ctx.dice && "TreasureRoom::create called without dice");

	// Mark the area for the treasure room
	for (int y = room.row; y <= room.row_end(); y++)
	{
		for (int x = room.col; x <= room.col_end(); x++)
		{
			ctx.map->set_tile(Vector2D{ x, y }, TileType::FLOOR, 1);
		}
	}

	// Calculate the center of the room
	const Vector2D center{ room.center_col(), room.center_row() };

	// Generate treasure at the center of the room
	ctx.map->generate_treasure(center, ctx, ctx.levelManager->get_dungeon_level(), quality);

	// Spawn the Dungeon Warden — the named boss guarding the vault.
	// Try the room center first, then spiral outward to find a free tile.
	{
		std::optional<Vector2D> wardenPos;
		constexpr int MAX_WARDEN_TRIES = 50;
		for (int t = 0; t < MAX_WARDEN_TRIES && !wardenPos; ++t)
		{
			Vector2D candidate{
				ctx.dice->roll(room.col, room.col_end()),
				ctx.dice->roll(room.row, room.row_end())
			};
			if (ctx.map->can_walk(candidate, ctx) && ctx.map->get_actor(candidate, ctx) == nullptr)
			{
				wardenPos = candidate;
			}
		}
		if (wardenPos)
		{
			MonsterParams wardenParams = MonsterCreator::get_params("dungeon_warden");
			wardenParams.name = DungeonNames::generate_warden_name(generationRng);
			ctx.creatures->push_back(
				MonsterCreator::create_from_params(*wardenPos, wardenParams, ctx));
		}
	}

	// Add guardians or traps based on quality
	int guardianCount = 0;
	switch (quality)
	{

	case 1:
	{
		guardianCount = ctx.dice->roll(0, 1);
		break;
	}

	case 2:
	{
		guardianCount = ctx.dice->roll(1, 2);
		break;
	}

	case 3:
	{
		guardianCount = ctx.dice->roll(2, 3);
		break;
	}

	}

	for (int i = 0; i < guardianCount; i++)
	{
		std::optional<Vector2D> guardPos;
		constexpr int MAX_GUARD_TRIES = 50;
		for (int t = 0; t < MAX_GUARD_TRIES; ++t)
		{
			Vector2D candidate{
				ctx.dice->roll(room.col, room.col_end()),
				ctx.dice->roll(room.row, room.row_end())
			};
			if (candidate != center &&
				ctx.map->can_walk(candidate, ctx) &&
				ctx.map->get_actor(candidate, ctx) == nullptr &&
				ctx.map->find_decoration_at(candidate, ctx) == nullptr)
			{
				guardPos = candidate;
				break;
			}
		}
		if (!guardPos)
		{
			continue;
		}

		ctx.map->add_monster(*guardPos, ctx);
	}

	setup_guard(room, ctx);

	if (ctx.messageSystem)
	{
		ctx.messageSystem->log(std::format(
			"Created treasure room at ({},{}) size {}x{} quality {}",
			room.col,
			room.row,
			room.width,
			room.height,
			quality));
	}
}

bool TreasureRoom::maybe_create(int dungeonLevel, RandomDice& generationRng, GameContext& ctx)
{
	assert(ctx.map && "TreasureRoom::maybe_create called without a map");
	assert(ctx.dice && "TreasureRoom::maybe_create called without dice");
	assert(ctx.rooms && "TreasureRoom::maybe_create called without rooms");

	const int treasureRoomChance = std::min(30 + (dungeonLevel * 5), 60);
	if (ctx.dice->d100() > treasureRoomChance)
	{
		return false;
	}

	// Need at least 2 rooms to skip the player's starting room
	if (ctx.rooms->size() < 2)
	{
		return false;
	}

	// Collect candidate rooms: single-entrance, not the player's starting room,
	// and not the room that already contains the stairs. Locking a staircase
	// inside the treasure room traps the player on this level permanently.

	auto is_valid_treasure_room = [&](int roomIndex) -> bool
	{
		assert(ctx.stairs != nullptr);
		const DungeonRoom& room = ctx.rooms->at(roomIndex);
		if (room.contains(ctx.stairs->position.x, ctx.stairs->position.y))
		{
			return false;
		}
		return count_entrances(*ctx.map, room) == 1;
	};

	auto singleEntranceIndices = std::views::iota(1, static_cast<int>(ctx.rooms->size())) 
		| std::views::filter(is_valid_treasure_room) 
		| std::ranges::to<std::vector>();

	if (singleEntranceIndices.empty())
	{
		return false;
	}

	const int pick = ctx.dice->roll(0, static_cast<int>(singleEntranceIndices.size()) - 1);
	const DungeonRoom& room = ctx.rooms->at(singleEntranceIndices[pick]);

	// Determine the quality of the treasure room based on dungeon level and luck
	int quality = 1;
	const int qualityRoll = ctx.dice->d100();
	if (qualityRoll <= 5 + dungeonLevel)
	{
		quality = 3;
	}
	else if (qualityRoll <= 15 + (dungeonLevel * 2))
	{
		quality = 2;
	}

	create(room, quality, generationRng, ctx);

	return true;
}
