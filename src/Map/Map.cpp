// file: Map.cpp
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <format>
#include <limits>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <raylib.h>

#include "../Actor/Creature.h"
#include "../Actor/InventoryOperations.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Factories/ItemCreator.h"
#include "../Factories/ItemFactory.h"
#include "../Factories/MonsterCreator.h"
#include "../Factories/MonsterFactory.h"
#include "../Persistent/Persistent.h"
#include "../Random/RandomDice.h"
#include "../Renderer/Renderer.h"
#include "../Systems/EncounterPlanner.h"
#include "../Systems/LevelManager.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/TileConfig.h"
#include "../Tools/DecorEditor.h"
#include "../Tools/PrefabLibrary.h"
#include "../Utils/Vector2D.h"
#include "Decoration.h"
#include "DungeonGenerator.h"
#include "DungeonNames.h"
#include "DungeonRoom.h"
#include "FovMap.h"
#include "Map.h"
#include "../Objects/Trap.h"

namespace
{
	// Returns true if `target` is reachable from `start` without crossing a
	// locked door or a wall. Used to assert that the jailer spawn is on the
	// correct (corridor) side of the locked treasure room door.
	// Returns true if `target` is reachable from `start` without crossing a
	// locked door or a wall. Used to assert that the jailer spawn is on the
	// correct (corridor) side of the locked treasure room door.
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

	struct FovProperties
	{
		bool walkable{};
		bool transparent{};
	};

	// Single source of truth for how each tile type maps onto the FOV grid.
	// Used by both set_tile (runtime mutations) and Map::load (FOV rebuild).
	FovProperties fov_properties_for(TileType type) noexcept
	{
		switch (type)
		{
		case TileType::FLOOR:
		case TileType::CORRIDOR:
		case TileType::OPEN_DOOR:
		case TileType::WATER:
		{
			return { true, true };
		}
		case TileType::WALL:
		case TileType::CLOSED_DOOR:
		default:
		{
			return { false, false };
		}
		}
	}
}

//====
Map::Map(int mapWidth, int mapHeight)
	: mapHeight(mapHeight),
	  mapWidth(mapWidth),
	  monsterFactory(std::make_unique<MonsterFactory>()),
	  itemFactory(std::make_unique<ItemFactory>()),
	  dijkstraCosts(static_cast<size_t>(mapWidth) * mapHeight, std::numeric_limits<int>::max()),
	  fovMap(std::make_unique<FovMap>(mapWidth, mapHeight)),
	  seed(0)
{
}

bool Map::in_bounds(Vector2D pos) const noexcept
{
	bool result = pos.y >= 0 && pos.y < mapHeight && pos.x >= 0 && pos.x < mapWidth;
	// Logging moved to callers that have GameContext access
	return result;
}

void Map::init_tiles()
{
	tiles.clear();

	// FovMap must be reset alongside tiles so can_walk / is_wall see correct
	// state even when init_tiles is called standalone (e.g. in tests).
	// All FovCells default to walkable=false, transparent=false — matches WALL.
	fovMap = std::make_unique<FovMap>(mapWidth, mapHeight);

	for (int y = 0; y < mapHeight; y++)
	{
		for (int x = 0; x < mapWidth; x++)
		{
			tiles.emplace_back(Tile(Vector2D{ x, y }, TileType::WALL, 0));
		}
	}
}

//====
// We have to move the map initialization code out of the constructor
// for enabling loading the map from the file.
void Map::init(GameContext& ctx)
{
	init_tiles();  // resets tiles + fovMap to all-walls
	if (ctx.messageSystem)
	{
		ctx.messageSystem->log("Map::init: " + std::to_string(mapWidth) + "x" + std::to_string(mapHeight) + " tile grid reset");
	}
	seed = ctx.dice ? ctx.dice->roll(0, std::numeric_limits<int>::max()) : 0;
	mapRng = RandomDice{ static_cast<unsigned int>(seed) };

	// Register the active map key before rooms are generated so decoration
	// stamps inside create_room land in the correct DecorEditor bucket.
	if (ctx.decorEditor && ctx.levelManager)
	{
		ctx.decorEditor->set_active_map(seed, ctx.levelManager->get_dungeon_level());
	}

	generate_rooms(ctx);

	post_process_doors();

	if (ctx.levelManager && ctx.levelManager->get_dungeon_level() >= 1)
	{
		maybe_create_treasure_room(ctx.levelManager->get_dungeon_level(), ctx);
	}
	place_amulet(ctx);
}

void Map::generate_rooms(GameContext& ctx)
{
	DungeonGenerator gen;
	gen.generate(mapWidth, mapHeight, mapRng, ctx, *this);
	place_stairs(ctx);
}

void Map::load(const json& j)
{
	mapWidth = j.at("map_width").get<int>();
	mapHeight = j.at("map_height").get<int>();
	seed = j.at("seed").get<int>();

	tiles.clear();
	for (const auto& tileJson : j.at("tiles"))
	{
		Vector2D position(
			tileJson.at("position").at("y").get<int>(),
			tileJson.at("position").at("x").get<int>());

		TileType type = static_cast<TileType>(tileJson.at("type").get<int>());
		bool explored = tileJson.at("explored").get<bool>();
		double cost = tileJson.at("cost").get<double>();

		tiles.emplace_back(position, type, cost);
		tiles.back().explored = explored;
	}

	fovMap = std::make_unique<FovMap>(mapWidth, mapHeight);
	mapRng = RandomDice{ static_cast<unsigned int>(seed) };

	// Rebuild FOV grid from loaded tile data.
	for (const auto& tile : tiles)
	{
		const auto [walkable, transparent] = fov_properties_for(tile.type);
		fovMap->set_properties(tile.position.x, tile.position.y, walkable, transparent);
	}

	// Post-process to place doors after loading map
	post_process_doors();

	// Note: Logging requires GameContext access - moved to caller
}

void Map::save(json& j)
{
	j["map_width"] = mapWidth;
	j["map_height"] = mapHeight;
	j["seed"] = seed;

	j["tiles"] = json::array();
	for (const auto& tile : tiles)
	{
		j["tiles"].push_back(
			{ { "position", { { "y", tile.position.y }, { "x", tile.position.x } } },
				{ "type", static_cast<int>(tile.type) }, // TileType is an enum
				{ "explored", tile.explored },
				{ "cost", tile.cost } });
	}
}

bool Map::is_wall(Vector2D pos) const noexcept
{
	return !fovMap->is_walkable(pos.x, pos.y);
}

bool Map::is_explored(Vector2D pos) const noexcept
{
	if (!in_bounds(pos))
	{
		// Logging moved to callers with GameContext access
		return false;
	}
	size_t index = get_index(pos);
	if (index >= tiles.size())
	{
		// Logging moved to callers with GameContext access
		return false;
	}
	return tiles.at(index).explored;
}

void Map::set_explored(Vector2D pos)
{
	if (!in_bounds(pos))
	{
		return; // Can't set explored for out of bounds positions
	}
	tiles.at(get_index(pos)).explored = true;
}

bool Map::is_in_fov(Vector2D pos) const noexcept
{
	return fovMap->is_in_fov(pos.x, pos.y);
}

bool Map::is_water(Vector2D pos) const noexcept
{
	if (!in_bounds(pos))
	{
		return false; // Out of bounds positions are not water
	}
	return tiles.at(get_index(pos)).type == TileType::WATER;
}

TileType Map::get_tile_type(Vector2D pos) const noexcept
{
	if (!in_bounds(pos))
	{
		return TileType::WALL; // Out of bounds positions are treated as walls
	}
	return tiles.at(get_index(pos)).type;
}

// returns true if the player after a successful move
void Map::tile_action(Creature& owner, TileType tileType, GameContext& ctx)
{
	switch (tileType)
	{

	case TileType::WATER:
	{
		// Only called on successful water entry (with swim ability)
		if (ctx.messageSystem)
		{
			ctx.messageSystem->log("You are in water");
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You are in water", true);
		}
		break;
	}

	case TileType::WALL:
	{
		if (ctx.messageSystem)
		{
			ctx.messageSystem->log("You are against a wall");
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You are against a wall", true);
		}
		break;
	}

	case TileType::FLOOR:
	{
		// ctx.message_system->log("You are on the floor");
		// ctx.message_system->message(WHITE_BLACK_PAIR, "You are on the floor", true);
		break;
	}

	case TileType::CLOSED_DOOR:
	{
		if (ctx.messageSystem)
		{
			ctx.messageSystem->log("You are at a door");
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You are at a door", true);
		}
		break;
	}

	case TileType::CORRIDOR:
	{
		// ctx.message_system->log("You are in a corridor");
		// ctx.message_system->message(WHITE_BLACK_PAIR, "You are in a corridor", true);
		break;
	}

	default:
	{
		if (ctx.messageSystem)
		{
			ctx.messageSystem->log("You are in an unknown area");
		}
	}

	}
}

Decoration* Map::find_decoration_at(Vector2D pos, const GameContext& ctx) const noexcept
{
	if (!ctx.decorations)
	{
		return nullptr;
	}

	for (auto& d : *ctx.decorations)
	{
		if (d && !d->isBroken && d->position == pos)
		{
			return d.get();
		}
	}

	return nullptr;
}

bool Map::is_collision(Creature& owner, TileType tileType, Vector2D pos, GameContext& ctx)
{
	// if there is an actor at the position
	const auto& actor = get_actor(pos, ctx);
	if (actor)
	{
		return true;
	}

	if (auto* decor = find_decoration_at(pos, ctx))
	{
		if (decor->blocks_movement)
		{
			return true;
		}
	}

	switch (tileType)
	{

	case TileType::WATER:
	{
		/*return owner.has_state(ActorState::CAN_SWIM) ? false : true;*/
		return false;
	}

	case TileType::WALL:
	{
		return true;
	}

	case TileType::FLOOR:
	{
		return false;
	}

	case TileType::CLOSED_DOOR:
	{
		return true; // Closed doors block movement
	}

	case TileType::OPEN_DOOR:
	{
		return false; // Open doors don't block movement
	}

	case TileType::CORRIDOR:
	{
		return false;
	}

	default:
	{
		return true;
	}

	}
}

void Map::compute_fov(GameContext& ctx)
{
	if (ctx.messageSystem)
	{
		ctx.messageSystem->log("...Computing FOV...");
	}

	assert(ctx.player && "Map::compute_fov called without player");
	assert(
		ctx.player->position.x >= 0 && ctx.player->position.x < mapWidth &&
		ctx.player->position.y >= 0 && ctx.player->position.y < mapHeight &&
		"Map::compute_fov: player position out of map bounds");

	fovMap->compute_fov(ctx.player->position.x, ctx.player->position.y, FOV_RADIUS);
	rebuild_dijkstra_map({ ctx.player->position }, ctx);
}

void Map::update()
{
	for (auto& tile : tiles)
	{
		if (is_in_fov(tile.position))
		{
			set_explored(tile.position);
		}
	}
}

void Map::render(const GameContext& ctx) const
{
	if (tiles.empty() || !ctx.renderer)
	{
		return;
	}

	int tileSize = ctx.renderer->get_tile_size();
	int cameraX = ctx.renderer->get_camera_x();
	int cameraY = ctx.renderer->get_camera_y();
	int visibleCols = ctx.renderer->get_viewport_cols();
	int visibleRows = ctx.renderer->get_viewport_rows() - GUI_RESERVE_ROWS;

	int startCol = std::max(0, cameraX / tileSize);
	int startRow = std::max(0, cameraY / tileSize);
	int endCol = std::min(mapWidth, startCol + visibleCols + 2);
	int endRow = std::min(mapHeight, startRow + visibleRows + 2);

	// Cardinal neighbor definitions: offset + bitmask bit
	struct NeighborDef
	{
		Vector2D offset;
		uint8_t bit;
	};
	constexpr uint8_t northBit = 8;
	constexpr uint8_t eastBit = 4;
	constexpr uint8_t southBit = 2;
	constexpr uint8_t westBit = 1;
	const std::array<NeighborDef, 4> cardinals = { { { DIR_N, northBit },
		{ DIR_E, eastBit },
		{ DIR_S, southBit },
		{ DIR_W, westBit } } };

	auto build_mask = [&](Vector2D center, auto predicate) -> uint8_t
	{
		uint8_t mask = 0;
		for (const auto& dir : cardinals)
		{
			if (predicate(center + dir.offset))
			{
				mask |= dir.bit;
			}
		}
		return mask;
	};

	auto is_wall_or_door = [&](Vector2D pos) -> bool
	{
		if (!in_bounds(pos))
		{
			return false;
		}
		TileType tileType = get_tile_type(pos);
		return tileType == TileType::WALL || tileType == TileType::CLOSED_DOOR;
	};

	auto is_walkable = [&](Vector2D pos) -> bool
	{
		if (!in_bounds(pos))
		{
			return false;
		}
		TileType tileType = get_tile_type(pos);
		return tileType == TileType::FLOOR || tileType == TileType::CORRIDOR || tileType == TileType::OPEN_DOOR || tileType == TileType::WATER;
	};

	const std::array<Vector2D, 8> allDirs = { { DIR_NW, DIR_N, DIR_NE, DIR_W, DIR_E, DIR_SW, DIR_S, DIR_SE } };

	// A border wall is a wall/door with at least one walkable tile
	// in its 8 neighbors. Only border walls form the visible room outline.
	auto is_border_wall = [&](Vector2D pos) -> bool
	{
		if (!is_wall_or_door(pos))
		{
			return false;
		}
		for (const auto& direction : allDirs)
		{
			if (is_walkable(pos + direction))
			{
				return true;
			}
		}
		return false;
	};

	auto is_visible = [&](Vector2D pos) -> bool
	{
		return is_in_fov(pos) || is_explored(pos);
	};

	// Deterministic hash for decoration placement (stable per map seed).
	auto decor_hash = [&](int tileY, int tileX, int salt) -> unsigned int
	{
		unsigned int hashValue = static_cast<unsigned int>(
			tileY * 7919 + tileX * 6271 + static_cast<int>(seed) * 1013 + salt * 3571);
		hashValue ^= hashValue >> 16;
		hashValue *= 0x45d9f3bU;
		hashValue ^= hashValue >> 16;
		return hashValue;
	};

	// ---------------------------------------------------------------------------
	// Zone-based decoration system.
	// 20-tile grid cells match room scale -- most rooms fall within one zone.
	// Zones: 0=dungeon, 1=library, 2=armory, 3=storage, 4=chapel
	// ---------------------------------------------------------------------------
	constexpr int zoneChapel = 4;

	auto resolve_decor = [&](Vector2D pos, TileType tileType) -> TileRef
	{
		// Torches only -- the floor itself carries the visual weight.
		if (tileType != TileType::WALL)
		{
			return TileRef{};
		}

		bool floorSouth = is_walkable(pos + DIR_S);
		bool floorEast = is_walkable(pos + DIR_E);
		bool floorWest = is_walkable(pos + DIR_W);
		if (!floorSouth || floorEast || floorWest)
		{
			return TileRef{};
		}

		int rowPhase = static_cast<int>(decor_hash(pos.y, 0, 88) % 4);
		if ((pos.x % 4 + 4) % 4 != rowPhase)
		{
			return TileRef{};
		}

		int zone = static_cast<int>(decor_hash(pos.y / 20, pos.x / 20, 199) % 5);
		const auto& tileConfig = *ctx.tileConfig;
		return (zone == zoneChapel) ? tileConfig.get("TILE_CANDELABRA") : tileConfig.get("TILE_TORCH_1");
	};

	for (int row = startRow; row < endRow; row++)
	{
		for (int col = startCol; col < endCol; col++)
		{
			Vector2D pos{ col, row };
			if (!is_visible(pos))
			{
				continue;
			}

			// In FOV: full colour. Explored but not visible: dimmed memory tint.
			bool inFov = is_in_fov(pos);
			Color tint = inFov
				? Color{ 255, 255, 255, 255 }
				: Color{ 95, 90, 82, 255 };

			TileType type = get_tile_type(pos);
			TileRef tileRef{};

			switch (type)
			{

			case TileType::WALL:
			{
				if (!is_border_wall(pos))
				{
					continue; // Interior wall -- no walkable neighbor, render as void
				}
				// Two border walls connect only if they share a common walkable 8-neighbor.
				// A void wall sitting between a room wall and a corridor shares no walkable
				// neighbor with the room wall, so it cannot bleed into the room wall mask.
				// Corner walls (only diagonally adjacent to floor) still connect because
				// their shared floor tile IS a common 8-neighbor of both wall tiles.
				auto connected_wall = [&](Vector2D neighbor) -> bool
				{
					if (!is_border_wall(neighbor))
					{
						return false;
					}
					for (const auto& direction : allDirs)
					{
						Vector2D shared = pos + direction;
						if (!is_walkable(shared))
						{
							continue;
						}
						Vector2D diff = shared - neighbor;
						if (std::abs(diff.x) <= 1 && std::abs(diff.y) <= 1)
						{
							return true;
						}
					}
					return false;
				};
				tileRef = Autotile::wall_resolve_mask(
					ctx.tileConfig->get_wall_autotile("WALL_AUTOTILE_STONE"),
					build_mask(pos, connected_wall));
				break;
			}

			case TileType::FLOOR:
			{
				tileRef = Autotile::resolve_mask(
					ctx.tileConfig->get_autotile("AUTOTILE_FLOOR_STONE"),
					build_mask(pos, is_walkable));
				break;
			}

			case TileType::CORRIDOR:
			{
				tileRef = ctx.tileConfig->get("TILE_CORRIDOR");
				break;
			}

			case TileType::WATER:
			{
				tileRef = ctx.tileConfig->get("TILE_WATER");
				break;
			}

			case TileType::CLOSED_DOOR:
			{
				TileRef floorRef = Autotile::resolve_mask(
					ctx.tileConfig->get_autotile("AUTOTILE_FLOOR_STONE"),
					build_mask(pos, is_walkable));
				ctx.renderer->draw_tile(Vector2D{ col, row }, floorRef, tint);

				// Check if door is locked and render differently
				if (is_door_locked(pos))
				{
					tileRef = ctx.tileConfig->get("TILE_DOOR_LOCKED");
				}
				else
				{
					tileRef = ctx.tileConfig->get("TILE_DOOR_CLOSED");
				}
				break;
			}

			case TileType::OPEN_DOOR:
			{
				TileRef floorRef = Autotile::resolve_mask(
					ctx.tileConfig->get_autotile("AUTOTILE_FLOOR_STONE"),
					build_mask(pos, is_walkable));
				ctx.renderer->draw_tile(Vector2D{ col, row }, floorRef, tint);
				int offset = ctx.renderer->get_tile_size() / 2;
				ctx.renderer->draw_tile_offset(
					Vector2D{ col, row },
					-offset,
					0,
					ctx.tileConfig->get("TILE_DOOR_OPEN"),
					tint);
				continue;
			}

			default:
			{
				continue;
			}

			}

			ctx.renderer->draw_tile(Vector2D{ col, row }, tileRef, tint);

			// Decorations: hand-placed overrides take priority over procedural.
			{
#ifndef EMSCRIPTEN
				TileRef decorRef = ctx.decorEditor
					? ctx.decorEditor->get_override(col, row)
					: TileRef{};
#else
				TileRef decorRef = resolve_decor(pos, type);
#endif
				if (!decorRef.is_valid())
				{
					decorRef = resolve_decor(pos, type);
				}
				if (decorRef.is_valid())
				{
					ctx.renderer->draw_tile(Vector2D{ col, row }, decorRef, tint);
				}
			}
		}
	}
}

void Map::add_item(Vector2D pos, GameContext& ctx)
{
	// 75% chance to spawn an item at all
	if (mapRng.roll(1, 100) > 75)
	{
		return;
	}

	// Use our ItemFactory to create a random item
	if (ctx.levelManager)
	{
		itemFactory->spawn_random_item(pos, ctx, ctx.levelManager->get_dungeon_level());
	}
}

void Map::dig(Vector2D begin, Vector2D end)
{
	if (begin.x > end.x)
	{
		std::swap(begin.x, end.x);
	}
	if (begin.y > end.y)
	{
		std::swap(begin.y, end.y);
	}

	for (int tileY = begin.y; tileY <= end.y; tileY++)
	{
		for (int tileX = begin.x; tileX <= end.x; tileX++)
		{
			set_tile(Vector2D{ tileX, tileY }, TileType::FLOOR, 1);
			fovMap->set_properties(tileX, tileY, true, true);
		}
	}
}

void Map::dig_corridor(Vector2D begin, Vector2D end)
{
	const int x1 = begin.x;
	const int y1 = begin.y;
	const int x2 = end.x;
	const int y2 = end.y;

	const bool horizontal_first = mapRng.roll(0, 1) == 1;

	// Only carve WALL tiles -- floor/corridor tiles keep their existing type.
	// This prevents corridors from visually scarring room interiors when the
	// MST path passes through another room's bounding box.
	auto dig_one = [&](Vector2D pos)
	{
		if (!in_bounds(pos))
		{
			return;
		}

		if (get_tile_type(pos) == TileType::WALL)
		{
			set_tile(pos, TileType::CORRIDOR, 1);
		}
		fovMap->set_properties(pos.x, pos.y, true, true);
	};

	if (horizontal_first)
	{
		for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x)
		{
			dig_one(Vector2D{ x, y1 });
		}
		for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y)
		{
			if (y == y1)
			{
				continue;
			}
			dig_one(Vector2D{ x2, y });
		}
	}
	else
	{
		for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y)
		{
			dig_one(Vector2D{ x1, y });
		}
		for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x)
		{
			if (x == x1)
			{
				continue;
			}
			dig_one(Vector2D{ x, y2 });
		}
	}
}

void Map::set_door(Vector2D thisTile, int tileX, int tileY, bool locked)
{
	set_tile(thisTile, TileType::CLOSED_DOOR, 2);
	fovMap->set_properties(tileX, tileY, false, false);

	size_t tileIndex = get_index(thisTile);
	if (tileIndex < tiles.size())
	{
		tiles[tileIndex].doorState = locked
			? DoorState::CLOSED_LOCKED
			: DoorState::CLOSED_UNLOCKED;
	}
}

void Map::set_tile(Vector2D pos, TileType newType, double cost)
{
	if (!in_bounds(pos))
	{
		return;
	}
	const size_t idx = get_index(pos);
	tiles.at(idx).type = newType;
	tiles.at(idx).cost = cost;

	// Keep fovMap in sync so is_wall / can_walk / A* see the change immediately.
	const auto [walkable, transparent] = fov_properties_for(newType);
	fovMap->set_properties(pos.x, pos.y, walkable, transparent);
}

void Map::create_room(const DungeonRoom& room, bool first, GameContext& ctx)
{
	if (ctx.rooms)
	{
		ctx.rooms->push_back(room);
	}

	dig(Vector2D{ room.col, room.row }, Vector2D{ room.col_end(), room.row_end() });

	// Stamp prefab decorations before water so water can see occupied tiles.
	if (ctx.prefabLibrary && ctx.decorEditor)
	{
		ctx.prefabLibrary->apply_to_room(room, *ctx.decorEditor, *this);

		// Register a blocking Decoration for every stamped tile.
		if (ctx.decorations)
		{
			for (int dy = room.row; dy <= room.row_end(); ++dy)
			{
				for (int dx = room.col; dx <= room.col_end(); ++dx)
				{
					TileRef t = ctx.decorEditor->get_override(dx, dy);
					if (!t.is_valid())
					{
						continue;
					}
					auto d = std::make_unique<Decoration>();
					d->position = Vector2D{ dx, dy };
					d->tile = t;
					d->name = "decoration";
					d->hp = 3;
					d->blocks_movement = true;
					d->lootTableKey = "";
					d->isBroken = false;
					ctx.decorations->push_back(std::move(d));
				}
			}
		}
	}

	spawn_water(room, ctx);

	if (first)
	{
		spawn_player(room, ctx);
	}

	spawn_barrels(room, ctx);
	spawn_items(room, ctx);
	spawn_traps(room, ctx);
	plan_encounter(room, ctx);
}

void Map::spawn_traps(const DungeonRoom& room, GameContext& ctx)
{
	assert(ctx.dice && "Map::spawn_traps called without dice");
	assert(ctx.tileConfig && "Map::spawn_traps called without tileConfig");
	assert(ctx.objects && "Map::spawn_traps called without objects");

	// ~30% of rooms get 0-2 random traps (was 10%)
	if (ctx.dice->d10() > 3)
	{
		return;  // Room has no traps
	}

	// Room gets 1-2 random traps
	int trapCount = ctx.dice->roll(1, 2);

	for (int i = 0; i < trapCount; ++i)
	{
		// Pick random valid floor tile in room
		Vector2D trapPos;
		bool foundSpot = false;

		for (int attempt = 0; attempt < 20; ++attempt)
		{
			trapPos.x = room.col + ctx.dice->roll(0, room.width - 1);
			trapPos.y = room.row + ctx.dice->roll(0, room.height - 1);

			if (!in_bounds(trapPos))
			{
				continue;
			}

			TileType tileType = get_tile_type(trapPos);

			// Must be a floor tile (walkable, no objects/items)
			if (tileType == TileType::FLOOR)
			{
				foundSpot = true;
				break;
			}
		}

		if (!foundSpot)
		{
			continue;  // Couldn't find a valid spot for this trap
		}

		// Create trap (randomly choose type)
		TrapType trapType;
		int trapChoice = ctx.dice->roll(1, 3);
		switch (trapChoice)
		{
		case 1:
			trapType = TrapType::PIT;
			break;
		case 2:
			trapType = TrapType::DART;
			break;
		case 3:
			trapType = TrapType::ARROW;
			break;
		default:
			trapType = TrapType::PIT;
		}

		auto trap = std::make_unique<Trap>(trapPos, trapType, *ctx.tileConfig);
		ctx.objects->push_back(std::move(trap));
	}
}

void Map::spawn_water(const DungeonRoom& room, GameContext& ctx)
{
	// Add water tiles
	constexpr int waterPercentage = 5; // 5% of tiles will be water (reduced from 10%)
	Vector2D waterPos{ 0, 0 };
	for (waterPos.y = room.row; waterPos.y <= room.row_end(); ++waterPos.y)
	{
		for (waterPos.x = room.col; waterPos.x <= room.col_end(); ++waterPos.x)
		{
			/*const int rolld100 = game->d.d100();*/
			const int rolld100 = mapRng.roll(1, 100);
			if (rolld100 < waterPercentage)
			{
				// Never place water on a decorated tile.
				if (ctx.decorEditor && ctx.decorEditor->get_override(waterPos.x, waterPos.y).is_valid())
				{
					continue;
				}
				// CRITICAL FIX: Check if water would block entrances using pattern matching
				if (!would_water_block_entrance(waterPos, ctx))
				{
					set_tile(waterPos, TileType::WATER, 10);
					fovMap->set_properties(waterPos.x, waterPos.y, true, true); // non-walkable and non-transparent
				}
			}
		}
	}
}

bool Map::would_water_block_entrance(Vector2D waterPos, GameContext& ctx) const
{
	// Pattern matching to prevent water from blocking entrances
	// Check all 8 directions around the water position for potential entrance patterns

	// Get surrounding positions
	std::vector<Vector2D> adjacent = {
		{ waterPos.x - 1, waterPos.y - 1 }, { waterPos.x, waterPos.y - 1 }, { waterPos.x + 1, waterPos.y - 1 }, // Top row    (NW, N, NE)
		{ waterPos.x - 1, waterPos.y },
		{ waterPos.x + 1, waterPos.y }, // Middle row (W, E)
		{ waterPos.x - 1, waterPos.y + 1 },
		{ waterPos.x, waterPos.y + 1 },
		{ waterPos.x + 1, waterPos.y + 1 } // Bottom row (SW, S, SE)
	};

	// Count walls around this position
	int wallCount = 0;

	for (const auto& pos : adjacent)
	{
		if (!in_bounds(pos))
		{
			wallCount++; // Out of bounds = wall
			continue;
		}

		if (get_tile_type(pos) == TileType::WALL)
		{
			wallCount++;
		}
	}

	// ENTRANCE PATTERN 1: Corner or edge position (high wall density)
	// If surrounded by 5+ walls, this is likely a corner or edge - safe for water
	if (wallCount >= 5)
	{
		return false; // Safe to place water
	}

	// ENTRANCE PATTERN 2: Doorway position (walls on opposite sides)
	// Check for corridor-like patterns: walls on opposite sides
	auto isWallOrOOB = [this](Vector2D pos)
	{
		return !in_bounds(pos) || get_tile_type(pos) == TileType::WALL;
	};

	// Check horizontal corridor pattern: W-F-W (wall-floor-wall)
	if (isWallOrOOB({ waterPos.x - 1, waterPos.y }) && isWallOrOOB({ waterPos.x + 1, waterPos.y }))
	{
		return true; // Would block horizontal passage
	}

	// Check vertical corridor pattern: W
	//                                    F
	//                                    W
	if (isWallOrOOB({ waterPos.x, waterPos.y - 1 }) && isWallOrOOB({ waterPos.x, waterPos.y + 1 }))
	{
		return true; // Would block vertical passage
	}

	// ENTRANCE PATTERN 3: Near room edge with potential door placement
	// Check if this position could be part of a future door placement
	// Look for patterns where this floor tile is adjacent to potential door areas

	// Check for L-shaped entrance patterns (common near room corners)
	int adjacentWalls = 0;
	int adjacentFloors = 0;

	// Count direct adjacent (not diagonal) walls and floors
	std::vector<Vector2D> directAdjacent = {
		{ waterPos.x, waterPos.y - 1 }, // North
		{ waterPos.x, waterPos.y + 1 }, // South
		{ waterPos.x - 1, waterPos.y }, // West
		{ waterPos.x + 1, waterPos.y } // East
	};

	for (const auto& pos : directAdjacent)
	{
		if (!in_bounds(pos))
		{
			adjacentWalls++;
			continue;
		}

		TileType tileType = get_tile_type(pos);
		if (tileType == TileType::WALL)
		{
			adjacentWalls++;
		}
		else if (tileType == TileType::FLOOR)
		{
			adjacentFloors++;
		}
	}

	// ENTRANCE PATTERN 4: Potential door position
	// If this position has exactly 2 walls and 2 floors adjacent, it might be a door spot
	if (adjacentWalls == 2 && adjacentFloors == 2)
	{
		// Check if walls are opposite each other (forming a corridor)
		bool hasOppositeWalls =
			(isWallOrOOB({ waterPos.x, waterPos.y - 1 }) && isWallOrOOB({ waterPos.x, waterPos.y + 1 })) ||
			(isWallOrOOB({ waterPos.x - 1, waterPos.y }) && isWallOrOOB({ waterPos.x + 1, waterPos.y }));

		if (hasOppositeWalls)
		{
			return true; // Would block potential door
		}
	}

	// ENTRANCE PATTERN 5: Room boundary positions
	// If this floor tile is on the edge of the room, it might be where a corridor connects
	if (ctx.rooms)
	{
		for (const DungeonRoom& room : *ctx.rooms)
		{
			if (!room.contains(waterPos.x, waterPos.y))
			{
				continue;
			}

			// On the floor perimeter -- corridor likely connects here.
			const bool on_edge =
				waterPos.x == room.col || waterPos.x == room.col_end() ||
				waterPos.y == room.row || waterPos.y == room.row_end();

			if (on_edge && adjacentWalls >= 1 && adjacentFloors >= 1)
			{
				return true;
			}
			break;
		}
	}

	// If none of the blocking patterns match, water is safe to place
	return false;
}

void Map::spawn_items(const DungeonRoom& room, GameContext& ctx)
{
	const int numItems = ctx.dice ? ctx.dice->roll(0, MAX_ROOM_ITEMS) : 0;
	for (int i = 0; i < numItems; i++)
	{
		Vector2D itemPos{ ctx.dice->roll(room.col, room.col_end()), ctx.dice->roll(room.row, room.row_end()) };
		constexpr int MAX_ITEM_TRIES = 20;
		int itemTries = 0;
		while (itemTries < MAX_ITEM_TRIES &&
			(!can_walk(itemPos, ctx) || is_stairs(itemPos, ctx) || find_decoration_at(itemPos, ctx) != nullptr))
		{
			itemPos.x = ctx.dice->roll(room.col, room.col_end());
			itemPos.y = ctx.dice->roll(room.row, room.row_end());
			++itemTries;
		}
		if (itemTries >= MAX_ITEM_TRIES)
		{
			continue;
		}
		add_item(itemPos, ctx);
	}
}

void Map::spawn_barrels(const DungeonRoom& room, GameContext& ctx)
{
	if (!ctx.decorations || !ctx.tileConfig)
	{
		return;
	}

	constexpr int MAX_ROOM_BARRELS = 2;
	const int count = mapRng.roll(0, MAX_ROOM_BARRELS);
	const TileRef barrel_tile = ctx.tileConfig->get("TILE_BARREL");

	for (int i = 0; i < count; ++i)
	{
		Vector2D pos{ mapRng.roll(room.col, room.col_end()), mapRng.roll(room.row, room.row_end()) };
		constexpr int MAX_TRIES = 20;
		int tries = 0;
		while (tries < MAX_TRIES && (!can_walk(pos, ctx) || find_decoration_at(pos, ctx) != nullptr))
		{
			pos.x = mapRng.roll(room.col, room.col_end());
			pos.y = mapRng.roll(room.row, room.row_end());
			++tries;
		}
		if (tries >= MAX_TRIES)
		{
			continue;
		}

		auto barrel = std::make_unique<Decoration>();
		barrel->position = pos;
		barrel->tile = barrel_tile;
		barrel->name = "barrel";
		barrel->hp = 2;
		barrel->blocks_movement = true;
		barrel->lootTableKey = "gold";
		ctx.decorations->push_back(std::move(barrel));
	}
}

void Map::spawn_player(const DungeonRoom& room, GameContext& ctx)
{
	assert(ctx.player && "Map::spawn_player called without player");
	assert(ctx.dice && "Map::spawn_player called without dice");

	Vector2D pos{ ctx.dice->roll(room.col, room.col_end()), ctx.dice->roll(room.row, room.row_end()) };
	constexpr int MAX_PLAYER_TRIES = 50;
	int playerTries = 0;
	while (playerTries < MAX_PLAYER_TRIES &&
		(!can_walk(pos, ctx) || find_decoration_at(pos, ctx) != nullptr))
	{
		pos.x = ctx.dice->roll(room.col, room.col_end());
		pos.y = ctx.dice->roll(room.row, room.row_end());
		++playerTries;
	}
	ctx.player->position = pos;
}

void Map::place_stairs(GameContext& ctx)
{
	assert(ctx.stairs && "Map::place_stairs called without stairs");
	assert(ctx.rooms && !ctx.rooms->empty() && "Map::place_stairs called without rooms");
	assert(ctx.dice && "Map::place_stairs called without dice");

	// BFS from room[0] to find the deepest room — forces player to explore
	const std::vector<DungeonRoom>& rooms = *ctx.rooms;
	const int roomCount = static_cast<int>(rooms.size());

	std::vector<int> depth(roomCount, -1);
	std::queue<int> frontier;
	depth[0] = 0;
	frontier.push(0);

	while (!frontier.empty())
	{
		const int current = frontier.front();
		frontier.pop();
		for (int neighbor : rooms[current].adjacentRoomIndices)
		{
			if (depth[neighbor] == -1)
			{
				depth[neighbor] = depth[current] + 1;
				frontier.push(neighbor);
			}
		}
	}

	const int deepestIndex = static_cast<int>(
		std::ranges::max_element(depth) - depth.begin());

	const DungeonRoom& room = rooms[deepestIndex];
	Vector2D stairsPos{ ctx.dice->roll(room.col, room.col_end()), ctx.dice->roll(room.row, room.row_end()) };
	constexpr int MAX_STAIR_TRIES = 200;
	for (int attempt = 0; attempt < MAX_STAIR_TRIES; ++attempt)
	{
		if (can_walk(stairsPos, ctx) && find_decoration_at(stairsPos, ctx) == nullptr)
		{
			break;
		}
		stairsPos.x = ctx.dice->roll(room.col, room.col_end());
		stairsPos.y = ctx.dice->roll(room.row, room.row_end());
	}
	ctx.stairs->position = stairsPos;
}

bool Map::is_stairs(Vector2D pos, GameContext& ctx) const
{
	return ctx.stairs && ctx.stairs->position == pos;
}

bool Map::can_walk(Vector2D pos, const GameContext& ctx) const noexcept
{
	if (is_wall(pos))
	{
		return false;
	}

	if (get_tile_type(pos) == TileType::CLOSED_DOOR)
	{
		return false;
	}

	if (get_actor(pos, ctx) != nullptr)
	{
		return false;
	}

	if (find_decoration_at(pos, ctx) != nullptr)
	{
		return false;
	}

	return true;
}

void Map::add_monster(Vector2D pos, GameContext& ctx) const
{
	// Use the monster factory to create a monster appropriate for the current dungeon level
	if (ctx.levelManager)
	{
		monsterFactory->spawn_random_monster(pos, ctx.levelManager->get_dungeon_level(), ctx);

		// Log the spawn for debugging
		Creature* monster = get_actor(pos, ctx);
		if (monster && ctx.messageSystem)
		{
			ctx.messageSystem->log("Spawned " + monster->actorData.name + " at level " + std::to_string(ctx.levelManager->get_dungeon_level()));
		}
	}
}

// getActor returns the actor at the given coordinates or `nullptr` if there's none
Creature* Map::get_actor(Vector2D pos, const GameContext& ctx) const noexcept
{
	// Check player position first
	if (ctx.player && ctx.player->position == pos)
	{
		return ctx.player;
	}

	// Check all creatures (excluding dead ones - they shouldn't block)
	for (const auto& actor : *ctx.creatures)
	{
		if (actor && actor->position == pos)
		{
			// Only return living creatures as blockers
			if (!actor->is_dead())
			{
				return actor.get();
			}
		}
	}

	return nullptr;
}

std::vector<std::vector<Tile>> Map::get_map() const noexcept
{
	std::vector<std::vector<Tile>> map;
	for (const auto& tile : tiles)
	{
		map.push_back({ tile });
	}
	return map;
}

// reveal map
void Map::reveal()
{
	for (auto& tile : tiles)
	{
		tile.explored = true;
	}
}

// regenerate map
void Map::regenerate(GameContext& ctx)
{
	// Renderer handles frame clearing
	// clear the actors container except the player and the stairs
	if (ctx.creatures)
	{
		ctx.creatures->clear();
	}
	if (ctx.inventoryData)
	{
		ctx.inventoryData->items.clear();
	}
	if (ctx.rooms)
	{
		ctx.rooms->clear(); // we clear the room coordinates
	}
	if (ctx.objects)
	{
		ctx.objects->clear();
	}
	if (ctx.decorations)
	{
		ctx.decorations->clear();
	}

	// generate a new map at current window dimensions (keep old size if curses not active)
	const int newH = get_map_height();
	const int newW = get_map_width();
	if (newH > 0 && newW > 0)
	{
		mapHeight = newH;
		mapWidth = newW;
	}
	init(ctx);
}

std::vector<Vector2D> Map::neighbors(Vector2D id, const GameContext& ctx, Vector2D target)
{
	std::vector<Vector2D> results;

	for (Vector2D dir : DIRS)
	{
		Vector2D next{ id.x + dir.x, id.y + dir.y };
		if (in_bounds(next))
		{
			// Allow target position even if occupied (for pathfinding to reach goal)
			// TODO: target uses x==-1 as sentinel for "no target"; replace with std::optional<Vector2D> at call sites
			bool isTarget = (target.x != -1 && next == target);
			if (isTarget)
			{
				// Allow closed doors as target; exclude only solid walls
				if (!is_wall(next) || get_tile_type(next) == TileType::CLOSED_DOOR)
				{
					results.push_back(next);
				}
			}
			else if (can_walk(next, ctx))
			{
				results.push_back(next);
			}
		}
	}

	if ((id.x + id.y) % 2 == 0)
	{
		// see "Ugly paths" section for an explanation:
		std::reverse(results.begin(), results.end());
	}

	return results;
}

double Map::cost(Vector2D fromNode, Vector2D toNode, const GameContext& ctx)
{
	// if there is an actor on the tile, return a high cost
	if (get_actor(toNode, ctx) != nullptr)
	{
		return 1000.0;
	}

	double baseCost = get_cost(toNode);

	// Check if this is a diagonal move (8-directional grid)
	int dx = std::abs(toNode.x - fromNode.x);
	int dy = std::abs(toNode.y - fromNode.y);

	if (dx == 1 && dy == 1) // Diagonal move
	{
		return baseCost * 1.414213562373095; // sqrt(2)
	}

	return baseCost;
}

double Map::get_cost(Vector2D pos) const noexcept
{
	if (!in_bounds(pos))
	{
		// Logging moved to callers with GameContext access
		return 1000.0; // High cost for out of bounds
	}
	size_t index = get_index(pos);
	if (index >= tiles.size())
	{
		// Logging moved to callers with GameContext access
		return 1000.0;
	}
	return tiles.at(index).cost;
}

std::vector<Vector2D> Map::bresenham_line(Vector2D from, Vector2D to)
{
	std::vector<Vector2D> path;

	int x0 = from.x;
	int y0 = from.y;
	const int x1 = to.x;
	const int y1 = to.y;

	const int dx = std::abs(x1 - x0);
	const int dy = std::abs(y1 - y0);
	const int sx = (x0 < x1) ? 1 : -1;
	const int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	while (x0 != x1 || y0 != y1)
	{
		int e2 = 2 * err;
		if (e2 > -dy)
		{
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx)
		{
			err += dx;
			y0 += sy;
		}
		path.push_back(Vector2D{ x0, y0 });
	}

	return path;
}

bool Map::has_los(Vector2D from, Vector2D to) const noexcept
{
	for (const auto& pos : bresenham_line(from, to))
	{
		// Skip endpoints
		if ((pos.x != from.x || pos.y != from.y) && (pos.x != to.x || pos.y != to.y))
		{
			if (is_wall(pos))
			{
				return false;
			}
		}
	}
	return true;
}

bool Map::is_door(Vector2D pos) const noexcept
{
	if (!is_in_bounds(pos))
		return false;

	TileType tileType = get_tile_type(pos);
	return tileType == TileType::CLOSED_DOOR || tileType == TileType::OPEN_DOOR;
}

bool Map::is_open_door(Vector2D pos) const noexcept
{
	if (pos.y < 0 || pos.y >= mapHeight || pos.x < 0 || pos.x >= mapWidth)
		return false;
	return get_tile_type(pos) == TileType::OPEN_DOOR;
}

bool Map::open_door(Vector2D pos, GameContext& ctx)
{
	if (!is_door(pos))
	{
		return false;
	}

	// Check if the door is already open
	if (get_tile_type(pos) == TileType::OPEN_DOOR)
	{
		return false;
	}

	// Check if the door is locked (can't open locked doors)
	if (is_door_locked(pos))
	{
		return false;
	}

	set_tile(pos, TileType::OPEN_DOOR, 1);

	// Clear door state when opening
	size_t tileIndex = get_index(pos);
	if (tileIndex < tiles.size())
	{
		tiles[tileIndex].doorState = DoorState::OPEN;
	}

	fovMap->set_properties(pos.x, pos.y, true, true);

	if (ctx.player && ctx.player->get_tile_distance(pos) <= FOV_RADIUS)
	{
		compute_fov(ctx);
	}

	return true;
}

bool Map::close_door(Vector2D pos, GameContext& ctx)
{
	if (!is_door(pos))
	{
		return false;
	}

	// Check if the door is already closed
	if (get_tile_type(pos) == TileType::CLOSED_DOOR)
	{
		return false;
	}

	// Check if there's an actor on the door - can't close if occupied
	if (get_actor(pos, ctx) != nullptr)
	{
		return false;
	}

	// Check if there's a floor item on the door tile
	if (ctx.inventoryData)
	{
		auto has_item_at_pos = [&pos](const std::unique_ptr<Item>& item)
		{
			return item && item->position == pos;
		};
		if (std::ranges::any_of(ctx.inventoryData->items, has_item_at_pos))
		{
			return false;
		}
	}

	// Change the tile type to DOOR (closed)
	set_tile(pos, TileType::CLOSED_DOOR, 2);

	// New doors are unlocked by default
	size_t tileIndex = get_index(pos);
	if (tileIndex < tiles.size())
	{
		tiles[tileIndex].doorState = DoorState::CLOSED_UNLOCKED;
	}

	// Make the tile non-walkable and non-transparent
	fovMap->set_properties(pos.x, pos.y, false, false);

	if (ctx.player && ctx.player->get_tile_distance(pos) <= FOV_RADIUS)
	{
		compute_fov(ctx);
	}

	return true;
}

bool Map::unlock_door(Vector2D pos, GameContext& ctx)
{
	if (!is_door(pos) || get_tile_type(pos) != TileType::CLOSED_DOOR)
	{
		return false;
	}

	size_t tileIndex = get_index(pos);
	if (tileIndex >= tiles.size())
	{
		return false;
	}

	if (tiles[tileIndex].doorState != DoorState::CLOSED_LOCKED)
	{
		return false; // Door is not locked
	}

	tiles[tileIndex].doorState = DoorState::CLOSED_UNLOCKED;
	return true;
}

void Map::open_all_room_doors(Vector2D doorPos, GameContext& ctx)
{
	// Find the room this door belongs to, then open every locked door of that room.
	// A door belongs to a room iff one of its cardinal neighbors is inside room.contains().
	if (!ctx.rooms)
	{
		unlock_door(doorPos, ctx);
		open_door(doorPos, ctx);
		return;
	}

	for (const auto& room : *ctx.rooms)
	{
		bool belongsToRoom = false;
		for (Vector2D dir : { DIR_N, DIR_S, DIR_E, DIR_W })
		{
			Vector2D nb = doorPos + dir;
			if (room.contains(nb.x, nb.y))
			{
				belongsToRoom = true;
				break;
			}
		}
		if (!belongsToRoom)
		{
			continue;
		}

		// Open every locked door on this room's wall border.
		for (int y = room.top_wall(); y <= room.bottom_wall(); ++y)
		{
			for (int x = room.left_wall(); x <= room.right_wall(); ++x)
			{
				Vector2D pos{ x, y };
				if (!in_bounds(pos) || room.contains(x, y))
				{
					continue;
				}
				if (!is_door_locked(pos))
				{
					continue;
				}

				auto opens_this_room = [&]()
				{
					for (Vector2D dir : { DIR_N, DIR_S, DIR_E, DIR_W })
					{
						if (room.contains((pos + dir).x, (pos + dir).y))
						{
							return true;
						}
					}
					return false;
				};

				if (opens_this_room())
				{
					unlock_door(pos, ctx);
					open_door(pos, ctx);
				}
			}
		}
		return;
	}

	// Fallback: no room found, open only this door.
	unlock_door(doorPos, ctx);
	open_door(doorPos, ctx);
}

bool Map::is_door_locked(Vector2D pos) const noexcept
{
	if (!in_bounds(pos))
	{
		return false;
	}

	size_t tileIndex = get_index(pos);
	if (tileIndex >= tiles.size())
	{
		return false;
	}

	return tiles[tileIndex].doorState == DoorState::CLOSED_LOCKED;
}

void Map::place_amulet(GameContext& ctx)
{
	// Only place the amulet on the final level
	assert(ctx.levelManager && "Map::place_amulet called without levelManager");
	assert(ctx.player && "Map::place_amulet called without player");
	assert(ctx.rooms && "Map::place_amulet called without rooms");
	assert(ctx.dice && "Map::place_amulet called without dice");

	if (ctx.levelManager->get_dungeon_level() == FINAL_DUNGEON_LEVEL)
	{
		// Choose a random room for the amulet
		const int index = ctx.dice->roll(0, static_cast<int>(ctx.rooms->size()) - 1);
		const DungeonRoom& room = ctx.rooms->at(index);

		// Find a walkable position in the room
		Vector2D amuletPos{ ctx.dice->roll(room.col, room.col_end()), ctx.dice->roll(room.row, room.row_end()) };
		while (!can_walk(amuletPos, ctx) || is_stairs(amuletPos, ctx))
		{
			amuletPos.x = ctx.dice->roll(room.col, room.col_end());
			amuletPos.y = ctx.dice->roll(room.row, room.row_end());
		}

		// Create and place the amulet
		InventoryOperations::add_item(*ctx.inventoryData, ItemCreator::create("amulet_of_yendor", amuletPos, *ctx.contentRegistry));

		// Log the placement (debug info)
		if (ctx.messageSystem)
		{
			ctx.messageSystem->log("Placed Amulet of Yendor at " + std::to_string(amuletPos.x) + "," + std::to_string(amuletPos.y));

			// Add a hint message
			ctx.messageSystem->message(RED_YELLOW_PAIR, "You sense a powerful artifact somewhere on this level...", true);
		}
	}
}

std::vector<MonsterPercentage> Map::get_monster_distribution(int dungeonLevel)
{
	return monsterFactory->get_current_distribution(dungeonLevel);
}

std::vector<ItemPercentage> Map::get_item_distribution(int dungeonLevel)
{
	return itemFactory->get_current_distribution(dungeonLevel);
}

void Map::create_treasure_room(const DungeonRoom& room, int quality, GameContext& ctx)
{
	assert(ctx.levelManager && "Map::create_treasure_room called without levelManager");
	assert(ctx.dice && "Map::create_treasure_room called without dice");

	// Mark the area for the treasure room
	for (int y = room.row; y <= room.row_end(); y++)
	{
		for (int x = room.col; x <= room.col_end(); x++)
		{
			set_tile(Vector2D{ x, y }, TileType::FLOOR, 1);
			fovMap->set_properties(x, y, true, true);
		}
	}

	// Calculate the center of the room
	const Vector2D center{ room.center_col(), room.center_row() };

	// Generate treasure at the center of the room
	itemFactory->generate_treasure(center, ctx, ctx.levelManager->get_dungeon_level(), quality);

	// Spawn the Dungeon Warden — the named boss guarding the vault.
	// Try the room center first, then spiral outward to find a free tile.
	{
		Vector2D wardenPos{ -1, -1 };
		constexpr int MAX_WARDEN_TRIES = 50;
		for (int t = 0; t < MAX_WARDEN_TRIES && wardenPos.x < 0; ++t)
		{
			Vector2D candidate{
				ctx.dice->roll(room.col, room.col_end()),
				ctx.dice->roll(room.row, room.row_end())
			};
			if (can_walk(candidate, ctx) && get_actor(candidate, ctx) == nullptr)
			{
				wardenPos = candidate;
			}
		}
		if (wardenPos.x >= 0)
		{
			MonsterParams wardenParams = MonsterCreator::get_params("dungeon_warden");
			wardenParams.name = DungeonNames::generate_warden_name(mapRng);
			ctx.creatures->push_back(
				MonsterCreator::create_from_params(wardenPos, wardenParams, ctx));
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
		Vector2D guardPos{ -1, -1 };
		constexpr int MAX_GUARD_TRIES = 50;
		for (int t = 0; t < MAX_GUARD_TRIES; ++t)
		{
			Vector2D candidate{
				ctx.dice->roll(room.col, room.col_end()),
				ctx.dice->roll(room.row, room.row_end())
			};
			if (candidate != center &&
				can_walk(candidate, ctx) &&
				get_actor(candidate, ctx) == nullptr &&
				find_decoration_at(candidate, ctx) == nullptr)
			{
				guardPos = candidate;
				break;
			}
		}
		if (guardPos.x < 0)
		{
			continue;
		}

		add_monster(guardPos, ctx);
	}

	setup_treasure_room_guard(room, ctx);

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

int Map::count_room_entrances(const DungeonRoom& room) const
{
	int count = 0;
	for (int y = room.top_wall(); y <= room.bottom_wall(); ++y)
	{
		for (int x = room.left_wall(); x <= room.right_wall(); ++x)
		{
			const Vector2D pos{ x, y };
			if (!in_bounds(pos) || room.contains(x, y))
			{
				continue;
			}
			if (get_tile_type(pos) != TileType::CLOSED_DOOR)
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

void Map::setup_treasure_room_guard(const DungeonRoom& room, GameContext& ctx)
{
	assert(ctx.contentRegistry && "Map::setup_treasure_room_guard called without contentRegistry");
	assert(ctx.creatures && "Map::setup_treasure_room_guard called without creatures");
	assert(ctx.dice && "Map::setup_treasure_room_guard called without dice");
	assert(ctx.player && "Map::setup_treasure_room_guard called without player");

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
			if (!in_bounds(doorPos) || room.contains(x, y))
			{
				continue;
			}
			if (get_tile_type(doorPos) != TileType::CLOSED_DOOR)
			{
				continue;
			}
			const Vector2D inward = find_inward_dir(doorPos);
			if (inward.x == 0 && inward.y == 0)
			{
				continue;
			}
			tiles[get_index(doorPos)].doorState = DoorState::CLOSED_LOCKED;
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
			if (!in_bounds(doorPos) || room.contains(x, y))
			{
				continue;
			}
			if (!is_door_locked(doorPos))
			{
				continue;
			}

			const Vector2D inward = find_inward_dir(doorPos);
			if (inward.x == 0 && inward.y == 0)
			{
				continue;
			}

			Vector2D spawnPos{ -1, -1 };
			constexpr int MAX_WALK = 5;
			for (int step = 1; step <= MAX_WALK; ++step)
			{
				const Vector2D candidate = doorPos - (inward * step);
				if (!in_bounds(candidate))
				{
					break; // left the map
				}
				const TileType tileType = get_tile_type(candidate);
				if (tileType == TileType::WALL)
				{
					break; // solid obstruction — corridor ends here
				}
				// Skip temporarily-occupied or non-floor tiles; keep stepping.
				if (tileType == TileType::CLOSED_DOOR)
				{
					continue;
				}
				if (get_actor(candidate, ctx) != nullptr)
				{
					continue;
				}
				if (find_decoration_at(candidate, ctx) != nullptr)
				{
					continue;
				}
				spawnPos = candidate;
				break;
			}

			if (spawnPos.x < 0)
			{
				continue;
			}

			candidates.push_back({ doorPos, spawnPos });
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
				if (!in_bounds(pos) || room.contains(x, y))
				{
					continue;
				}
				if (is_door_locked(pos))
				{
					tiles[get_index(pos)].doorState = DoorState::CLOSED_UNLOCKED;
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

	const Vector2D playerPos = ctx.player->position;
	const bool stairsAvailable = ctx.stairs != nullptr;

	const DoorCandidate* best = nullptr;
	int bestDist = std::numeric_limits<int>::max();

	for (const DoorCandidate& c : candidates)
	{
		if (stairsAvailable &&
			!is_reachable_without_locked_doors(c.spawnPos, ctx.stairs->position, *this))
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
		is_reachable_without_locked_doors(best->spawnPos, ctx.stairs->position, *this)) &&
		"setup_treasure_room_guard: reachability filter passed but assert disagrees");

	auto jailer = MonsterCreator::create_from_params(
		best->spawnPos,
		MonsterCreator::get_params("dungeon_jailer"),
		ctx);

	auto key = ItemCreator::create("dungeon_key", best->spawnPos, *ctx.contentRegistry);
	InventoryOperations::add_item_to_inventory(jailer->inventoryData, std::move(key), *jailer);

	ctx.creatures->push_back(std::move(jailer));
}

bool Map::maybe_create_treasure_room(int dungeonLevel, GameContext& ctx)
{
	assert(ctx.dice && "Map::maybe_create_treasure_room called without dice");
	assert(ctx.rooms && "Map::maybe_create_treasure_room called without rooms");

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
	std::vector<int> singleEntranceIndices;

	for (int i = 1; i < static_cast<int>(ctx.rooms->size()); ++i)
	{
		const DungeonRoom& candidate = ctx.rooms->at(i);
		if (ctx.stairs != nullptr &&
			candidate.contains(ctx.stairs->position.x, ctx.stairs->position.y))
		{
			continue; // never lock the staircase room
		}
		if (count_room_entrances(candidate) == 1)
		{
			singleEntranceIndices.push_back(i);
		}
	}

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

	create_treasure_room(room, quality, ctx);

	return true;
}

void Map::post_process_doors()
{
	auto isRoom = [this](Vector2D pos)
	{
		return in_bounds(pos) && (get_tile_type(pos) == TileType::FLOOR || get_tile_type(pos) == TileType::WATER);
	};
	auto isWall = [this](Vector2D pos)
	{
		return in_bounds(pos) && (get_tile_type(pos) == TileType::WALL || get_tile_type(pos) == TileType::WATER);
	};

	for (int y = 0; y < mapHeight; ++y)
	{
		for (int x = 0; x < mapWidth; ++x)
		{
			Vector2D pos{ x, y };
			if (get_tile_type(pos) == TileType::CORRIDOR)
			{
				int roomNeighbors = 0;
				int wallNeighbors = 0;

				for (Vector2D dir : { DIR_N, DIR_S, DIR_E, DIR_W })
				{
					Vector2D neighborPos = pos + dir;
					if (!in_bounds(neighborPos))
					{
						continue;
					}

					if (isRoom(neighborPos))
					{
						roomNeighbors++;
					}
					else if (isWall(neighborPos))
					{
						wallNeighbors++;
					}
				}

				if (roomNeighbors >= 1 && wallNeighbors >= 2)
				{
					// Get 3x3 grid around current position
					Vector2D upLeft = { pos.y - 1, pos.x - 1 };
					Vector2D up = { pos.y - 1, pos.x };
					Vector2D upRight = { pos.y - 1, pos.x + 1 };
					Vector2D left = { pos.y, pos.x - 1 };
					Vector2D right = { pos.y, pos.x + 1 };
					Vector2D downLeft = { pos.y + 1, pos.x - 1 };
					Vector2D down = { pos.y + 1, pos.x };
					Vector2D downRight = { pos.y + 1, pos.x + 1 };

					// Exclude pattern RRR/WCC/WWW (and rotations)
					bool shouldExclude = false;

					// Original: RRR/WCC/WWW
					if (isRoom(upLeft) && isRoom(up) && isRoom(upRight) &&
						isWall(left) && get_tile_type(right) == TileType::CORRIDOR &&
						isWall(downLeft) && isWall(down) && isWall(downRight))
					{
						shouldExclude = true;
					}

					// 90� rotation: WWR/CWR/CWR
					if (isWall(upLeft) && isWall(up) && isRoom(upRight) &&
						get_tile_type(left) == TileType::CORRIDOR && isRoom(right) &&
						get_tile_type(downLeft) == TileType::CORRIDOR && isWall(down) && isRoom(downRight))
					{
						shouldExclude = true;
					}

					// 180� rotation: WWW/CCW/RRR
					if (isWall(upLeft) && isWall(up) && isWall(upRight) &&
						get_tile_type(left) == TileType::CORRIDOR && isWall(right) &&
						isRoom(downLeft) && isRoom(down) && isRoom(downRight))
					{
						shouldExclude = true;
					}

					// 270� rotation: RWC/RWC/RWW
					if (isRoom(upLeft) && isWall(up) && get_tile_type(upRight) == TileType::CORRIDOR &&
						isRoom(left) && get_tile_type(right) == TileType::CORRIDOR &&
						isRoom(downLeft) && isWall(down) && isWall(downRight))
					{
						shouldExclude = true;
					}

					// Exclude WRR/WCC/WWC pattern
					if (isWall(upLeft) && isRoom(up) && isRoom(upRight) &&
						isWall(left) && get_tile_type(right) == TileType::CORRIDOR &&
						isWall(downLeft) && isWall(down) && get_tile_type(downRight) == TileType::CORRIDOR)
					{
						shouldExclude = true;
					}

					// Exclude WRw/WCC/WWW pattern (where w = water)
					if (isWall(upLeft) && isRoom(up) && get_tile_type(upRight) == TileType::WATER &&
						isWall(left) && get_tile_type(right) == TileType::CORRIDOR &&
						isWall(downLeft) && isWall(down) && isWall(downRight))
					{
						shouldExclude = true;
					}

					if (shouldExclude)
					{
						continue; // Skip placing door for excluded patterns
					}

					// Check for WCW/RDW/RWW pattern (move door UP)
					if (isWall(upLeft) && get_tile_type(up) == TileType::CORRIDOR && isWall(upRight) &&
						isRoom(left) && isWall(right) &&
						isRoom(downLeft) && isWall(down) && isWall(downRight))
					{
						// Move door UP
						Vector2D upPos = { pos.y - 1, pos.x };
						set_door(upPos, upPos.x, upPos.y, false);
					}
					// Check for W.w/CDW/WWW pattern (move door UP) - where . = water or corridor
					else if (isWall(upLeft) &&
						(get_tile_type(up) == TileType::WATER || get_tile_type(up) == TileType::CORRIDOR) &&
						get_tile_type(upRight) == TileType::WATER &&
						get_tile_type(left) == TileType::CORRIDOR && isWall(right) &&
						isWall(downLeft) && isWall(down) && isWall(downRight))
					{
						// Move door UP
						Vector2D upPos = { pos.y - 1, pos.x };
						set_door(upPos, upPos.x, upPos.y, false);
					}
					// Check for Z-pattern: WRR/CCW/WWW (move door left)
					else if (isWall(upLeft) && isRoom(up) && isRoom(upRight) &&
						get_tile_type(left) == TileType::CORRIDOR && isWall(right) &&
						isWall(downLeft) && isWall(down) && isWall(downRight))
					{
						// Move door LEFT
						Vector2D leftPos = { pos.y, pos.x - 1 };
						set_door(leftPos, leftPos.x, leftPos.y, false);
					}
					else
					{
						// All other doors (including WRR/CCR/WRR pattern)
						set_door(pos, pos.x, pos.y, false);
					}
				}
			}
		}
	}
}

void Map::spawn_all_enhanced_items_debug(Vector2D position, GameContext& ctx)
{
	itemFactory->spawn_all_enhanced_items_debug(position, ctx);
}

// ---- place_from_graph: render dungeon graph (rooms + edges) to tiles ----
// Idempotent: can regenerate from the same room graph without data loss.
// Separates graph construction from tile rendering.

void Map::place_from_graph(
	const std::vector<DungeonRoom>& rooms,
	GameContext& ctx)
{
	if (ctx.messageSystem)
	{
		ctx.messageSystem->log("place_from_graph: Starting with " + std::to_string(rooms.size()) + " rooms");
	}

	// Step 1: Create all rooms (dig them, spawn water/items/player)
	for (int i = 0; i < static_cast<int>(rooms.size()); ++i)
	{
		create_room(rooms[i], i == 0, ctx);
	}

	if (ctx.messageSystem)
	{
		ctx.messageSystem->log("place_from_graph: Rooms created, now connecting with corridors");
	}

	// Step 2: Connect adjacent rooms with corridors (from graph edges)
	std::set<std::pair<int, int>> dug_edges;

	for (int a_idx = 0; a_idx < static_cast<int>(rooms.size()); ++a_idx)
	{
		const DungeonRoom& a = rooms[a_idx];

		for (int b_idx : a.adjacentRoomIndices)
		{
			// Avoid digging the same corridor twice
			std::pair<int, int> edge_forward(std::min(a_idx, b_idx), std::max(a_idx, b_idx));
			if (dug_edges.count(edge_forward))
			{
				continue;
			}
			dug_edges.insert(edge_forward);

			const DungeonRoom& b = rooms[b_idx];

			// Determine relative positions and dig L-shape corridor
			const bool a_below = a.top_wall() > b.bottom_wall();
			const bool a_right = a.left_wall() > b.right_wall();
			const bool a_above = a.bottom_wall() < b.top_wall();
			const bool a_left = a.right_wall() < b.left_wall();

			if (a_below)
			{
				// a is below b -- corridor rises from a, crosses horizontally, drops to b
				const int mid = (b.bottom_wall() + a.top_wall()) / 2;
				dig_corridor(Vector2D{ a.center_col(), a.top_wall() }, Vector2D{ a.center_col(), mid });
				dig_corridor(Vector2D{ a.center_col(), mid }, Vector2D{ b.center_col(), mid });
				dig_corridor(Vector2D{ b.center_col(), mid }, Vector2D{ b.center_col(), b.bottom_wall() });
			}
			else if (a_right)
			{
				// a is to the right of b
				const int mid = (b.right_wall() + a.left_wall()) / 2;
				dig_corridor(Vector2D{ a.left_wall(), a.center_row() }, Vector2D{ mid, a.center_row() });
				dig_corridor(Vector2D{ mid, a.center_row() }, Vector2D{ mid, b.center_row() });
				dig_corridor(Vector2D{ mid, b.center_row() }, Vector2D{ b.right_wall(), b.center_row() });
			}
			else if (a_above)
			{
				// a is above b
				const int mid = (a.bottom_wall() + b.top_wall()) / 2;
				dig_corridor(Vector2D{ a.center_col(), a.bottom_wall() }, Vector2D{ a.center_col(), mid });
				dig_corridor(Vector2D{ a.center_col(), mid }, Vector2D{ b.center_col(), mid });
				dig_corridor(Vector2D{ b.center_col(), mid }, Vector2D{ b.center_col(), b.top_wall() });
			}
			else if (a_left)
			{
				// a is to the left of b
				const int mid = (a.right_wall() + b.left_wall()) / 2;
				dig_corridor(Vector2D{ a.right_wall(), a.center_row() }, Vector2D{ mid, a.center_row() });
				dig_corridor(Vector2D{ mid, a.center_row() }, Vector2D{ mid, b.center_row() });
				dig_corridor(Vector2D{ mid, b.center_row() }, Vector2D{ b.left_wall(), b.center_row() });
			}
			else
			{
				// Rooms are adjacent or overlapping -- direct L-shape between centres.
				dig_corridor(
					Vector2D{ a.center_col(), a.center_row() },
					Vector2D{ b.center_col(), b.center_row() });
			}
		}
	}

	if (ctx.messageSystem)
	{
		ctx.messageSystem->log("place_from_graph: Finished. Dug " + std::to_string(dug_edges.size()) + " edges");
	}
}

int Map::get_dijkstra_cost(Vector2D pos) const noexcept
{
	if (!in_bounds(pos))
	{
		return std::numeric_limits<int>::max();
	}

	return dijkstraCosts[static_cast<size_t>(pos.y) * mapWidth + pos.x];
}

void Map::rebuild_dijkstra_map(const std::vector<Vector2D>& goals, const GameContext& ctx)
{
	std::fill(dijkstraCosts.begin(), dijkstraCosts.end(), std::numeric_limits<int>::max());

	struct FrontierEntry
	{
		int cost{ 0 };
		Vector2D position;
		bool operator>(const FrontierEntry& rhs) const noexcept { return cost > rhs.cost; }
	};
	std::priority_queue<FrontierEntry, std::vector<FrontierEntry>, std::greater<FrontierEntry>> frontier;

	for (const Vector2D& goal : goals)
	{
		if (!in_bounds(goal))
		{
			continue;
		}
		const size_t goalIndex = static_cast<size_t>(goal.y) * mapWidth + goal.x;
		dijkstraCosts[goalIndex] = 0;
		frontier.push(FrontierEntry{ 0, goal });
	}

	if (frontier.empty())
	{
		return;
	}

	static const std::vector<Vector2D> cardinalAndDiagonal = {
		DIR_N, DIR_S, DIR_W, DIR_E, DIR_NW, DIR_NE, DIR_SW, DIR_SE
	};

	while (!frontier.empty())
	{
		const FrontierEntry top = frontier.top();
		frontier.pop();

		const size_t currentIndex = static_cast<size_t>(top.position.y) * mapWidth + top.position.x;
		if (top.cost > dijkstraCosts[currentIndex])
		{
			continue;
		}

		for (const Vector2D& delta : cardinalAndDiagonal)
		{
			Vector2D next{ top.position.x + delta.x, top.position.y + delta.y };
			if (!in_bounds(next))
			{
				continue;
			}
			if (!fovMap->is_walkable(next.x, next.y))
			{
				continue;
			}
			if (find_decoration_at(next, ctx) != nullptr)
			{
				continue;
			}
			const size_t nextIndex = static_cast<size_t>(next.y) * mapWidth + next.x;
			const int newCost = top.cost + 1;
			if (newCost < dijkstraCosts[nextIndex])
			{
				dijkstraCosts[nextIndex] = newCost;
				frontier.push(FrontierEntry{ newCost, next });
			}
		}
	}
}

// end of file: Map.cpp
