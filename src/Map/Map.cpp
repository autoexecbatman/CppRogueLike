// file: Map.cpp
#include <algorithm>
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
#include "../Actor/Stairs.h"
#include "../ActorTypes/Player.h"
#include "../Actor/Destructible.h"
#include "../Actor/InventoryOperations.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Factories/ItemCreator.h"
#include "../Systems/ContentRegistry.h"
#include "../Factories/ItemFactory.h"
#include "../Factories/MonsterFactory.h"
#include "../Items/ItemClassification.h"
#include "../Persistent/Persistent.h"
#include "../Random/RandomDice.h"
#include "../Renderer/Renderer.h"
#include "../Systems/EncounterPlanner.h"
#include "../Systems/LevelManager.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/TileConfig.h"
#include "../Tools/DecorEditor.h"
#include "../Utils/Vector2D.h"
#include "DungeonGenerator.h"
#include "DungeonRoom.h"
#include "FovMap.h"
#include "Map.h"

//====
Map::Map(int map_width, int map_height)
	: map_height(map_height),
	  map_width(map_width),
	  monsterFactory(std::make_unique<MonsterFactory>()),
	  itemFactory(std::make_unique<ItemFactory>()),
	  fovMap(std::make_unique<FovMap>(map_width, map_height)),
	  dijkstraCosts(static_cast<size_t>(map_width) * map_height, std::numeric_limits<int>::max()),
	  seed(0) // Will be set in init() with GameContext access
{
}

bool Map::in_bounds(Vector2D pos) const noexcept
{
	bool result = pos.y >= 0 && pos.y < map_height && pos.x >= 0 && pos.x < map_width;
	// Logging moved to callers that have GameContext access
	return result;
}

void Map::init_tiles(GameContext& ctx)
{
	if (!tiles.empty())
	{
		tiles.clear();
	}

	if (ctx.messageSystem)
	{
		ctx.messageSystem->log("init_tiles: Creating " + std::to_string(map_width * map_height) + " tiles (" + std::to_string(map_width) + " x " + std::to_string(map_height) + ")");
	}

	for (int y = 0; y < map_height; y++)
	{
		for (int x = 0; x < map_width; x++)
		{
			tiles.emplace_back(Tile(Vector2D{ x, y }, TileType::WALL, 0));
		}
	}

	if (ctx.messageSystem)
	{
		ctx.messageSystem->log("init_tiles: Created " + std::to_string(tiles.size()) + " tiles");
	}
}

//====
// We have to move the map initialization code out of the constructor
// for enabling loading the map from the file.
void Map::init(bool withActors, GameContext& ctx)
{
	init_tiles(ctx);
	seed = ctx.dice ? ctx.dice->roll(0, std::numeric_limits<int>::max()) : 0;
	mapRng_ = RandomDice{ static_cast<unsigned int>(seed) };
	fovMap = std::make_unique<FovMap>(map_width, map_height);
	generate_rooms(withActors, ctx);

	post_process_doors();

	if (ctx.levelManager && ctx.levelManager->get_dungeon_level() > 1)
	{
		maybe_create_treasure_room(ctx.levelManager->get_dungeon_level(), ctx);
	}
	place_amulet(ctx);
}

void Map::generate_rooms(bool withActors, GameContext& ctx)
{
	DungeonGenerator gen;
	gen.generate(map_width, map_height, mapRng_, withActors, ctx, *this);
	place_stairs(ctx);
}

void Map::load(const json& j)
{
	map_width = j.at("map_width").get<int>();
	map_height = j.at("map_height").get<int>();
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

	fovMap = std::make_unique<FovMap>(map_width, map_height);
	mapRng_ = RandomDice{ static_cast<unsigned int>(seed) };

	// Rebuild FOV grid from loaded tile data.
	for (const auto& tile : tiles)
	{
		bool walkable = false;
		bool transparent = false;

		switch (tile.type)
		{

		case TileType::FLOOR:
		case TileType::CORRIDOR:
		case TileType::OPEN_DOOR:
		{
			walkable = true;
			transparent = true;
			break;
		}

		case TileType::WATER:
		{
			walkable = true;
			transparent = true;
			break;
		}

		case TileType::WALL:
		case TileType::CLOSED_DOOR:
		{
			walkable = false;
			transparent = false;
			break;
		}

		default:
		{
			walkable = false;
			transparent = false;

			break;
		}

		}

		fovMap->set_properties(tile.position.x, tile.position.y, walkable, transparent);
	}

	// Post-process to place doors after loading map
	post_process_doors();

	// Note: Logging requires GameContext access - moved to caller
}

void Map::save(json& j)
{
	j["map_width"] = map_width;
	j["map_height"] = map_height;
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

Decoration* Map::find_decoration_at(Vector2D pos, GameContext& ctx) const noexcept
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

	// Safety check for valid player position
	if (!ctx.player ||
		ctx.player->position.x < 0 || ctx.player->position.x >= map_width ||
		ctx.player->position.y < 0 || ctx.player->position.y >= map_height)
	{
		if (ctx.messageSystem)
		{
			ctx.messageSystem->log("Warning: Can't compute FOV - invalid player position");
		}
		return;
	}

	fovMap->compute_fov(ctx.player->position.x, ctx.player->position.y, FOV_RADIUS);
	rebuild_dijkstra_map({ ctx.player->position });
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
	int endCol = std::min(map_width, startCol + visibleCols + 2);
	int endRow = std::min(map_height, startRow + visibleRows + 2);

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
	const std::array<NeighborDef, 4> cardinals = {{
		{ DIR_N, northBit },
		{ DIR_E, eastBit },
		{ DIR_S, southBit },
		{ DIR_W, westBit }
	}};

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


	const std::array<Vector2D, 8> allDirs = {{
		DIR_NW, DIR_N, DIR_NE, DIR_W, DIR_E, DIR_SW, DIR_S, DIR_SE
	}};

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
				tileRef = wall_autotile_resolve_mask(
					ctx.tileConfig->get_wall_autotile("WALL_AUTOTILE_STONE"),
					build_mask(pos, connected_wall));
				break;
			}
			case TileType::FLOOR:
			{
				tileRef = autotile_resolve_mask(
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
				TileRef floorRef = autotile_resolve_mask(
					ctx.tileConfig->get_autotile("AUTOTILE_FLOOR_STONE"),
					build_mask(pos, is_walkable));
				ctx.renderer->draw_tile(Vector2D{ col, row }, floorRef, tint);
				tileRef = ctx.tileConfig->get("TILE_DOOR_CLOSED");
				break;
			}
			case TileType::OPEN_DOOR:
			{
				TileRef floorRef = autotile_resolve_mask(
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
	if (mapRng_.roll(1, 100) > 75)
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

	const bool horizontal_first = mapRng_.roll(0, 1) == 1;

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

void Map::set_door(Vector2D thisTile, int tileX, int tileY)
{
	set_tile(thisTile, TileType::CLOSED_DOOR, 2);
	fovMap->set_properties(tileX, tileY, false, false);
}

void Map::set_tile(Vector2D pos, TileType newType, double cost)
{
	if (!in_bounds(pos))
	{
		return; // Can't set tile for out of bounds positions
	}
	tiles.at(get_index(pos)).type = newType;
	tiles.at(get_index(pos)).cost = cost;
}

void Map::create_room(const DungeonRoom& room, bool first, bool withActors, GameContext& ctx)
{
	if (ctx.rooms)
	{
		ctx.rooms->push_back(room);
	}

	dig(Vector2D{ room.col, room.row }, Vector2D{ room.col_end(), room.row_end() });

	spawn_water(room, ctx);

	if (!withActors)
	{
		return;
	}

	if (first)
	{
		spawn_player(room, ctx);
	}

	spawn_barrels(room, ctx);
	spawn_items(room, ctx);
	plan_encounter(room, ctx);
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
			const int rolld100 = mapRng_.roll(1, 100);
			if (rolld100 < waterPercentage)
			{
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
		while (!can_walk(itemPos, ctx) || is_stairs(itemPos, ctx))
		{
			itemPos.x = ctx.dice->roll(room.col, room.col_end());
			itemPos.y = ctx.dice->roll(room.row, room.row_end());
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
	const int count = mapRng_.roll(0, MAX_ROOM_BARRELS);
	const TileRef barrel_tile = ctx.tileConfig->get("TILE_BARREL");

	for (int i = 0; i < count; ++i)
	{
		Vector2D pos{ mapRng_.roll(room.col, room.col_end()), mapRng_.roll(room.row, room.row_end()) };
		constexpr int MAX_TRIES = 20;
		int tries = 0;
		while (tries < MAX_TRIES && (!can_walk(pos, ctx) || find_decoration_at(pos, ctx) != nullptr))
		{
			pos.x = mapRng_.roll(room.col, room.col_end());
			pos.y = mapRng_.roll(room.row, room.row_end());
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
	if (!ctx.player || !ctx.dice)
	{
		return;
	}

	Vector2D pos{ ctx.dice->roll(room.col, room.col_end()), ctx.dice->roll(room.row, room.row_end()) };
	while (!can_walk(pos, ctx))
	{
		pos.x = ctx.dice->roll(room.col, room.col_end());
		pos.y = ctx.dice->roll(room.row, room.row_end());
	}
	ctx.player->position = pos;
}

void Map::place_stairs(GameContext& ctx)
{
	if (!ctx.stairs || !ctx.rooms || ctx.rooms->empty() || !ctx.dice)
	{
		return;
	}

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
	while (!can_walk(stairsPos, ctx))
	{
		stairsPos.x = ctx.dice->roll(room.col, room.col_end());
		stairsPos.y = ctx.dice->roll(room.row, room.row_end());
	}
	ctx.stairs->position = stairsPos;
}

bool Map::is_stairs(Vector2D pos, GameContext& ctx) const
{
	return ctx.stairs && ctx.stairs->position == pos;
}

bool Map::can_walk(Vector2D pos, GameContext& ctx) const noexcept
{
	if (is_wall(pos)) // check if the tile is a wall
	{
		return false;
	}

	// Check for doors - can only walk through open doors
	if (get_tile_type(pos) == TileType::CLOSED_DOOR)
	{
		return false; // Closed doors block movement
	}

	// CRITICAL FIX: Check if there's already an actor at this position
	if (get_actor(pos, ctx) != nullptr)
	{
		return false; // Position occupied by another actor
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
Creature* Map::get_actor(Vector2D pos, GameContext& ctx) const noexcept
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
			if (actor->destructible && !actor->destructible->is_dead())
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
		map_height = newH;
		map_width = newW;
	}
	init(true, ctx);
}

std::vector<Vector2D> Map::neighbors(Vector2D id, GameContext& ctx, Vector2D target)
{
	std::vector<Vector2D> results;

	for (Vector2D dir : DIRS)
	{
		Vector2D next{ id.x + dir.x, id.y + dir.y };
		if (in_bounds(next))
		{
			// Allow target position even if occupied (for pathfinding to reach goal)
			bool isTarget = (target.x != -1 && next == target);
			if (isTarget)
			{
				// For target, only check terrain (walls, doors) not actors
				if (!is_wall(next) && get_tile_type(next) != TileType::CLOSED_DOOR)
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

double Map::cost(Vector2D fromNode, Vector2D toNode, GameContext& ctx)
{
	// if there is an actor on the tile, return a high cost
	if (get_actor(toNode, ctx) != nullptr)
	{
		return 1000.0;
	}
	
	double baseCost = get_cost(toNode, ctx);
	
	// Check if this is a diagonal move (8-directional grid)
	int dx = std::abs(toNode.x - fromNode.x);
	int dy = std::abs(toNode.y - fromNode.y);
	
	if (dx == 1 && dy == 1)  // Diagonal move
	{
		return baseCost * 1.414213562373095;  // sqrt(2)
	}
	
	return baseCost;
}

double Map::get_cost(Vector2D pos, GameContext& ctx) const noexcept
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
	if (pos.y < 0 || pos.y >= map_height || pos.x < 0 || pos.x >= map_width)
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

	set_tile(pos, TileType::OPEN_DOOR, 1);
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

	// Change the tile type to DOOR (closed)
	set_tile(pos, TileType::CLOSED_DOOR, 2);

	// Make the tile non-walkable and non-transparent
	fovMap->set_properties(pos.x, pos.y, false, false);

	if (ctx.player && ctx.player->get_tile_distance(pos) <= FOV_RADIUS)
	{
		compute_fov(ctx);
	}

	return true;
}

void Map::place_amulet(GameContext& ctx)
{
	// Only place the amulet on the final level
	if (!ctx.levelManager || !ctx.player || !ctx.rooms || !ctx.dice)
		return;

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
	if (!ctx.levelManager || !ctx.dice)
		return;

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
		Vector2D guardPos;
		do
		{
			guardPos.x = ctx.dice->roll(room.col, room.col_end());
			guardPos.y = ctx.dice->roll(room.row, room.row_end());
		} while ((guardPos == center) ||
			!can_walk(guardPos, ctx) ||
			get_actor(guardPos, ctx) != nullptr);

		add_monster(guardPos, ctx);
	}

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

bool Map::maybe_create_treasure_room(int dungeonLevel, GameContext& ctx)
{
	if (!ctx.dice || !ctx.rooms)
	{
		return false;
	}

	// Probability increases with dungeon level
	int treasureRoomChance = 5 + (dungeonLevel * 2); // 5% + 2% per level

	// Cap at 25%
	treasureRoomChance = std::min(treasureRoomChance, 25);

	if (ctx.dice->d100() > treasureRoomChance)
	{
		return false; // No treasure room this time
	}

	// Need at least 2 rooms to skip the player's starting room
	if (ctx.rooms->size() < 2)
	{
		return false;
	}

	// Select a random room, skipping index 0 (player's starting room)
	const int index = ctx.dice->roll(1, static_cast<int>(ctx.rooms->size()) - 1);
	const DungeonRoom& room = ctx.rooms->at(index);

	// Only use rooms that are large enough
	if (room.width < 6 || room.height < 6)
	{
		return false;
	}

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

	for (int y = 0; y < map_height; ++y)
	{
		for (int x = 0; x < map_width; ++x)
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
						set_door(upPos, upPos.x, upPos.y);
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
						set_door(upPos, upPos.x, upPos.y);
					}
					// Check for Z-pattern: WRR/CCW/WWW (move door left)
					else if (isWall(upLeft) && isRoom(up) && isRoom(upRight) &&
						get_tile_type(left) == TileType::CORRIDOR && isWall(right) &&
						isWall(downLeft) && isWall(down) && isWall(downRight))
					{
						// Move door LEFT
						Vector2D leftPos = { pos.y, pos.x - 1 };
						set_door(leftPos, leftPos.x, leftPos.y);
					}
					else
					{
						// All other doors (including WRR/CCR/WRR pattern)
						set_door(pos, pos.x, pos.y);
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
	bool withActors,
	GameContext& ctx)
{
	if (ctx.messageSystem)
	{
		ctx.messageSystem->log("place_from_graph: Starting with " + std::to_string(rooms.size()) + " rooms");
	}

	// Step 1: Create all rooms (dig them, spawn water/items/player)
	for (int i = 0; i < static_cast<int>(rooms.size()); ++i)
	{
		create_room(rooms[i], i == 0, withActors, ctx);
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
	return dijkstraCosts[static_cast<size_t>(pos.y) * map_width + pos.x];
}

void Map::rebuild_dijkstra_map(const std::vector<Vector2D>& goals)
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
		const size_t goalIndex = static_cast<size_t>(goal.y) * map_width + goal.x;
		dijkstraCosts[goalIndex] = 0;
		frontier.push(FrontierEntry{ 0, goal });
	}

	if (frontier.empty())
	{
		return;
	}

	static const std::vector<Vector2D> cardinalAndDiagonal =
	{
		DIR_N, DIR_S, DIR_W, DIR_E,
		DIR_NW, DIR_NE, DIR_SW, DIR_SE
	};

	while (!frontier.empty())
	{
		const FrontierEntry top = frontier.top();
		frontier.pop();

		const size_t currentIndex = static_cast<size_t>(top.position.y) * map_width + top.position.x;
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
			const size_t nextIndex = static_cast<size_t>(next.y) * map_width + next.x;
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
