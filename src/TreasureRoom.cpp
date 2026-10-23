// file: TreasureRoom.cpp
#include <algorithm>
#include <cassert>
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
#include "CreatureManager.h"
#include "DungeonNames.h"
#include "DungeonRoom.h"
#include "GameContext.h"
#include "InventoryOperations.h"
#include "ItemCreator.h"
#include "ItemFactory.h"
#include "LevelManager.h"
#include "Map.h"
#include "MessageSystem.h"
#include "MonsterCreator.h"
#include "MonsterRegistry.h"
#include "RandomDice.h"
#include "TileType.h"
#include "TreasureRoom.h"
#include "Vector2D.h"

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
		if (!map.is_in_bounds(pos))
		{
			return false;
		}
		if (map.is_door_locked(pos))
		{
			return false;
		}
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

		for (
			Vector2D dir :
			{ DIR_N, DIR_S, DIR_E, DIR_W, DIR_NE, DIR_NW, DIR_SE, DIR_SW })
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
} // namespace

int TreasureRoom::count_entrances(const Map& map, const DungeonRoom& room)
{
	int count = 0;
	for (int row = room.top_wall(); row <= room.bottom_wall(); ++row)
	{
		for (int col = room.left_wall(); col <= room.right_wall(); ++col)
		{
			const Vector2D pos{ col, row };
			if (!map.is_in_bounds(pos) || room.contains(col, row))
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
	assert(ctx.stairs && "TreasureRoom::setup_guard: called without stairs");
	assert(
		ctx.dataManager &&
		"TreasureRoom::setup_guard: called without a dataManager");
	assert(
		ctx.creatureManager &&
		"TreasureRoom::setup_guard: called without a creatureManager");
	assert(
		ctx.monsterRegistry &&
		"TreasureRoom::setup_guard: called without a monsterRegistry");
	assert(ctx.map && "TreasureRoom::setup_guard called without a map");
	assert(
		ctx.contentRegistry &&
		"TreasureRoom::setup_guard called without contentRegistry");
	assert(
		ctx.creatures && "TreasureRoom::setup_guard called without creatures");
	assert(ctx.dice && "TreasureRoom::setup_guard called without dice");
	assert(ctx.player() && "TreasureRoom::setup_guard called without player");

	// A door belongs to this room when a cardinal neighbour is inside it, whatever lies outside.

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
	for (int row = room.top_wall(); row <= room.bottom_wall(); ++row)
	{
		for (int col = room.left_wall(); col <= room.right_wall(); ++col)
		{
			Vector2D doorPos{ col, row };
			if (!ctx.map->is_in_bounds(doorPos) || room.contains(col, row))
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
			ctx.map->lock_door(doorPos);
		}
	}

	// Pass 2: collect jailer spawn candidates, walking out from each locked entrance up to MAX_WALK steps.
	// Only a wall or the map edge stops the walk; actors, decorations and other doors are passed.
	struct DoorCandidate
	{
		Vector2D doorPos;
		Vector2D spawnPos;
	};

	std::vector<DoorCandidate> candidates;

	for (int row = room.top_wall(); row <= room.bottom_wall(); ++row)
	{
		for (int col = room.left_wall(); col <= room.right_wall(); ++col)
		{
			Vector2D doorPos{ col, row };
			if (!ctx.map->is_in_bounds(doorPos) || room.contains(col, row))
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
					break; // solid obstruction - corridor ends here
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
		for (int row = room.top_wall(); row <= room.bottom_wall(); ++row)
		{
			for (int col = room.left_wall(); col <= room.right_wall(); ++col)
			{
				const Vector2D pos{ col, row };
				if (!ctx.map->is_in_bounds(pos) || room.contains(col, row))
				{
					continue;
				}
				ctx.map->unlock_door(pos);
			}
		}
	};

	if (candidates.empty())
	{
		unlock_all_room_entrances();
		return;
	}

	// With every door locked, keep the candidates still reachable from the stairs;
	// a dead end that only connects back through a locked door fails.
	const Vector2D playerPos = ctx.player()->position;
	const DoorCandidate* best = nullptr;
	int bestDist = std::numeric_limits<int>::max();

	for (const DoorCandidate& doorCandidate : candidates)
	{
		if (!is_reachable_without_locked_doors(
				doorCandidate.spawnPos,
				ctx.stairs->position,
				*ctx.map))
		{
			continue; // dead-end pocket - skip
		}

		const int dist =
			doorCandidate.spawnPos.manhattan_distance_to(playerPos);
		if (dist < bestDist)
		{
			bestDist = dist;
			best = &doorCandidate;
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
		is_reachable_without_locked_doors(
			best->spawnPos,
			ctx.stairs->position,
			*ctx.map) &&
		"TreasureRoom::setup_guard: reachability filter passed but assert disagrees");

	auto jailer = MonsterCreator::create_from_params(
		best->spawnPos,
		ctx.monsterRegistry->get_params("dungeon_jailer"),
		ctx);

	auto key = ItemCreator::create("dungeon_key", best->spawnPos, ctx);
	[[maybe_unused]] const auto giveKeyToJailerResult =
		InventoryOperations::add_item_to_inventory(
			jailer->inventoryData,
			std::move(key),
			*jailer,
			*ctx.dataManager);
	assert(
		giveKeyToJailerResult.has_value() &&
		"TreasureRoom::setup_guard: the jailer could not carry its key");

	ctx.creatureManager->add_creature(std::move(jailer), ctx);
}

void TreasureRoom::create(
	const DungeonRoom& room,
	int quality,
	RandomDice& generationRng,
	GameContext& ctx)
{
	assert(
		ctx.creatureManager &&
		"TreasureRoom::create: called without a creatureManager");
	assert(
		ctx.monsterRegistry &&
		"TreasureRoom::create: called without a monsterRegistry");
	assert(ctx.map && "TreasureRoom::create called without a map");
	assert(
		ctx.levelManager && "TreasureRoom::create called without levelManager");
	assert(ctx.dice && "TreasureRoom::create called without dice");

	// Drain the pools on the vault floor, so the treasure and its warden stand dry.
	// Only water is cleared: the box also holds shaped walls and clipped corridors.
	for (int row = room.row; row <= room.row_end(); row++)
	{
		for (int col = room.col; col <= room.col_end(); col++)
		{
			if (ctx.map->get_tile_type(Vector2D{ col, row }) == TileType::WATER)
			{
				ctx.map->set_tile(Vector2D{ col, row }, TileType::FLOOR);
			}
		}
	}

	// Calculate the center of the room
	const Vector2D center{ room.center_col(), room.center_row() };

	// Generate treasure at the center of the room
	ItemFactory::generate_treasure(
		center,
		ctx.levelManager->get_dungeon_level(),
		quality,
		ctx);

	// Spawn the Dungeon Warden - the named boss guarding the vault.
	// Try the room center first, then spiral outward to find a free tile.
	{
		std::optional<Vector2D> wardenPos;
		constexpr int MAX_WARDEN_TRIES = 50;
		for (
			int attempt = 0; attempt < MAX_WARDEN_TRIES && !wardenPos;
			++attempt)
		{
			Vector2D candidate{
				ctx.dice->roll(room.col, room.col_end()),
				ctx.dice->roll(room.row, room.row_end()),
			};
			if (ctx.map->can_place_at(candidate, ctx))
			{
				wardenPos = candidate;
			}
		}
		if (wardenPos)
		{
			MonsterParams wardenParams =
				ctx.monsterRegistry->get_params("dungeon_warden");
			wardenParams.name =
				DungeonNames::generate_warden_name(generationRng);
			ctx.creatureManager->add_creature(
				MonsterCreator::create_from_params(
					*wardenPos,
					wardenParams,
					ctx),
				ctx);
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

	for (int guardianIndex = 0; guardianIndex < guardianCount; guardianIndex++)
	{
		std::optional<Vector2D> guardPos;
		constexpr int MAX_GUARD_TRIES = 50;
		for (int attempt = 0; attempt < MAX_GUARD_TRIES; ++attempt)
		{
			Vector2D candidate{
				ctx.dice->roll(room.col, room.col_end()),
				ctx.dice->roll(room.row, room.row_end()),
			};
			if (candidate != center && ctx.map->can_place_at(candidate, ctx))
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

	assert(
		ctx.messageSystem &&
		"TreasureRoom::create: called without a messageSystem");
	ctx.messageSystem->log(
		std::format(
			"Created treasure room at ({},{}) size {}x{} quality {}",
			room.col,
			room.row,
			room.width,
			room.height,
			quality));
}

bool TreasureRoom::maybe_create(
	int dungeonLevel,
	RandomDice& generationRng,
	GameContext& ctx)
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

	// Candidates have one entrance and are neither the starting room nor the stairs' room:
	// stairs locked inside would trap the player on this level.

	auto is_valid_treasure_room = [&](int roomIndex) -> bool
	{
		assert(
			ctx.stairs != nullptr &&
			"TreasureRoom::maybe_create: called without stairs");
		const DungeonRoom& room = ctx.rooms->at(roomIndex);
		if (room.contains(ctx.stairs->position.x, ctx.stairs->position.y))
		{
			return false;
		}
		return count_entrances(*ctx.map, room) == 1;
	};

	auto singleEntranceIndices =
		std::views::iota(1, static_cast<int>(ctx.rooms->size())) |
		std::views::filter(is_valid_treasure_room) |
		std::ranges::to<std::vector>();

	if (singleEntranceIndices.empty())
	{
		return false;
	}

	const int pick =
		ctx.dice->roll(0, static_cast<int>(singleEntranceIndices.size()) - 1);
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
