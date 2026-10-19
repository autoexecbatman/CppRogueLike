// file: Map.cpp
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <queue>
#include <ranges>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <raylib.h>

#include "Colors.h"
#include "Creature.h"
#include "DecorEditor.h"
#include "Decoration.h"
#include "DungeonGenerator.h"
#include "DungeonRoom.h"
#include "EncounterPlanner.h"
#include "FovMap.h"
#include "GameContext.h"
#include "InventoryOperations.h"
#include "ItemCreator.h"
#include "ItemFactory.h"
#include "LevelManager.h"
#include "Map.h"
#include "MessageSystem.h"
#include "MonsterFactory.h"
#include "Persistent.h"
#include "PrefabLibrary.h"
#include "RandomDice.h"
#include "Renderer.h"
#include "SpellTile.h"
#include "TileConfig.h"
#include "Trap.h"
#include "TreasureRoom.h"
#include "Vector2D.h"
#include "Vision.h"

namespace
{
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
} // namespace

//====
Map::Map(int mapWidth, int mapHeight)
	: mapWidth(mapWidth),
	  mapHeight(mapHeight),
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

	// Reset with tiles, so is_unobstructed and is_wall agree when init_tiles runs alone, as tests run it.
	// A new FovCell is neither walkable nor transparent, which is a WALL.
	fovMap = std::make_unique<FovMap>(mapWidth, mapHeight);

	for (int row = 0; row < mapHeight; row++)
	{
		for (int col = 0; col < mapWidth; col++)
		{
			tiles.emplace_back(Tile(Vector2D{ col, row }, TileType::WALL));
		}
	}
}

//====
// We have to move the map initialization code out of the constructor
// for enabling loading the map from the file.
void Map::init(GameContext& ctx)
{
	assert(ctx.rooms && "Map::init: called without rooms");

	init_tiles(); // resets tiles + fovMap to all-walls
	assert(ctx.messageSystem && "Map::init: called without a messageSystem");
	ctx.messageSystem->log("Map::init: " + std::to_string(mapWidth) + "x" + std::to_string(mapHeight) + " tile grid reset");
	assert(ctx.dice && "Map::init: called without dice");
	seed = ctx.dice->roll(0, std::numeric_limits<int>::max());
	mapRng = RandomDice{ static_cast<unsigned int>(seed) };

	// Register the active map key before rooms are generated so decoration
	// stamps inside create_room land in the correct DecorEditor bucket.
	assert(ctx.levelManager && "Map::init: called without a levelManager");
	assert(ctx.decorEditor && "Map::init: called without a decorEditor");
	ctx.decorEditor->set_active_map(seed, ctx.levelManager->get_dungeon_level());

	generate_rooms(ctx);

	post_process_doors();

	if (ctx.levelManager->get_dungeon_level() >= 1)
	{
		TreasureRoom::maybe_create(ctx.levelManager->get_dungeon_level(), mapRng, ctx);
	}
	place_amulet(ctx);

	// Everything carved must be reachable, asked from the player's tile and never a room's centre,
	// which a moat prefab fills with rock.
	assert(!ctx.rooms->empty() && "Map::init generated no rooms");
	assert(ctx.player() && "Map::init finished without placing the player");
	assert(
		count_unreachable_tiles(ctx.player()->position) == 0 &&
		"Map::init left tiles the player cannot reach");
	assert(has_wall_border() && "Map::init carved into the map's edge");
}

void Map::generate_rooms(GameContext& ctx)
{
	DungeonGenerator gen;
	gen.generate(mapWidth, mapHeight, mapRng, ctx, *this);
	place_stairs(ctx);
}

void Map::load(const json& source)
{
	mapWidth = source.at("map_width").get<int>();
	mapHeight = source.at("map_height").get<int>();
	seed = source.at("seed").get<int>();

	tiles.clear();
	for (const auto& tileJson : source.at("tiles"))
	{
		Vector2D position(
			tileJson.at("position").at("x").get<int>(),
			tileJson.at("position").at("y").get<int>());

		TileType type = static_cast<TileType>(tileJson.at("type").get<int>());
		bool explored = tileJson.at("explored").get<bool>();

		tiles.emplace_back(position, type);
		tiles.back().explored = explored;
	}

	fovMap = std::make_unique<FovMap>(mapWidth, mapHeight);
	mapRng = RandomDice{ static_cast<unsigned int>(seed) };

	// Rebuild FOV grid from loaded tile data.
	for (const auto& tile : tiles)
	{
		fovMap->set_properties(tile.position.x, tile.position.y, fov_properties_for(tile.type));
	}

	// Post-process to place doors after loading map
	post_process_doors();

	// Note: Logging requires GameContext access - moved to caller
}

void Map::save(json& output)
{
	output["map_width"] = mapWidth;
	output["map_height"] = mapHeight;
	output["seed"] = seed;

	output["tiles"] = json::array();
	for (const auto& tile : tiles)
	{
		output["tiles"].push_back(
			{ { "position", { { "y", tile.position.y }, { "x", tile.position.x } } },
				{ "type", static_cast<int>(tile.type) }, // TileType is an enum
				{ "explored", tile.explored } });
	}
}

bool Map::is_wall(Vector2D pos) const noexcept
{
	return !fovMap->is_walkable(pos.x, pos.y);
}

int Map::count_unreachable_tiles(Vector2D start) const noexcept
{
	// Every door counts as a way through. is_wall reads the field-of-view grid, where a shut door is a wall.
	auto is_open = [this](Vector2D pos)
	{
		return get_tile_type(pos) != TileType::WALL;
	};

	if (!in_bounds(start) || !is_open(start))
	{
		return -1;
	}

	std::vector<bool> reached(tiles.size(), false);
	std::vector<Vector2D> pending;
	reached[get_index(start)] = true;
	pending.push_back(start);

	while (!pending.empty())
	{
		const Vector2D here = pending.back();
		pending.pop_back();

		// Four-way: the player moves on eight, but a diagonal pinched between two walls
		// is not a passage anything should depend on.
		const Vector2D steps[4] = {
			Vector2D{ here.x + 1, here.y }, Vector2D{ here.x - 1, here.y }, Vector2D{ here.x, here.y + 1 }, Vector2D{ here.x, here.y - 1 }
		};
		for (const Vector2D& next : steps)
		{
			if (!in_bounds(next) || reached[get_index(next)] || !is_open(next))
			{
				continue;
			}
			reached[get_index(next)] = true;
			pending.push_back(next);
		}
	}

	int unreachable = 0;
	for (const auto& tile : tiles)
	{
		if (tile.type != TileType::WALL && !reached[get_index(tile.position)])
		{
			++unreachable;
		}
	}
	return unreachable;
}

bool Map::has_wall_border() const noexcept
{
	// Top and bottom rows, then the left and right columns.
	for (int col = 0; col < mapWidth; ++col)
	{
		if (get_tile_type(Vector2D{ col, 0 }) != TileType::WALL || get_tile_type(Vector2D{ col, mapHeight - 1 }) != TileType::WALL)
		{
			return false;
		}
	}
	for (int row = 0; row < mapHeight; ++row)
	{
		if (get_tile_type(Vector2D{ 0, row }) != TileType::WALL || get_tile_type(Vector2D{ mapWidth - 1, row }) != TileType::WALL)
		{
			return false;
		}
	}
	return true;
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
// Tells the player about the tile they are standing on, where it is worth
// remarking on. Called after a successful move and when waiting in place.
//
// Only water has anything to say. A wall or a closed door cannot reach here:
// both are collisions, so a move onto one never completes, and the player's own
// tile is never either. Floor and corridor are unremarkable by design.
void Map::describe_tile(TileType tileType, GameContext& ctx)
{
	assert(ctx.tileConfig && "Map::describe_tile: called without a tileConfig");

	const TileDefinition& definition = ctx.tileConfig->get_tile_definition(tileType);

	// Most ground is unremarkable and says nothing.
	if (definition.entryMessage.empty())
	{
		return;
	}

	assert(ctx.messageSystem && "Map::describe_tile: called without a messageSystem");
	ctx.messageSystem->log(definition.entryMessage);
	ctx.messageSystem->message(ColorPairId::WHITE_BLACK, definition.entryMessage, MessageCompletion::FINISHED);
}

Decoration* Map::find_decoration_at(Vector2D pos, const GameContext& ctx) const noexcept
{
	assert(ctx.decorations && "Map::find_decoration_at: called without decorations");

	for (auto& decoration : *ctx.decorations)
	{
		assert(decoration && "find_decoration_at: the decoration list holds a null entry");

		if (!decoration->isBroken && decoration->position == pos)
		{
			return decoration.get();
		}
	}

	return nullptr;
}

void Map::compute_fov(GameContext& ctx)
{
	assert(ctx.messageSystem && "Map::compute_fov: called without a messageSystem");
	ctx.messageSystem->log("...Computing FOV...");

	assert(ctx.player() && "Map::compute_fov called without player");
	assert(
		ctx.player()->position.x >= 0 && ctx.player()->position.x < mapWidth &&
		ctx.player()->position.y >= 0 && ctx.player()->position.y < mapHeight &&
		"Map::compute_fov: player position out of map bounds");

	fovMap->compute_fov(ctx.player()->position.x, ctx.player()->position.y, Vision::sight_radius(*ctx.player()));
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
	assert(ctx.decorEditor && "Map::render: called without a decorEditor");
	assert(ctx.tileConfig && "Map::render: called without a tileConfig");

	if (tiles.empty() || !ctx.renderer)
	{
		return;
	}

	int tileSize = ctx.renderer->get_tile_size();
	int cameraX = ctx.renderer->get_camera_x();
	int cameraY = ctx.renderer->get_camera_y();
	int visibleCols = ctx.renderer->get_viewport_cols();
	int visibleRows = ctx.renderer->get_viewport_rows() - ctx.renderer->get_gui_reserve_rows();

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

	// Decoration zones on a 20-tile grid, about one room per cell.
	// Zones: 0=dungeon, 1=library, 2=armory, 3=storage, 4=chapel
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

			// In FOV: full colour. Explored but not visible: a neutral memory tint, since it multiplies
			// with the warm memory light quad and only darkness should separate the two.
			bool inFov = is_in_fov(pos);
			Color tint = inFov
				? Color{ 255, 255, 255, 255 }
				: Color{ 92, 92, 96, 255 };

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
				// Two border walls connect only through a shared walkable 8-neighbour, so a void wall
				// by a corridor cannot bleed into a room wall, while a corner wall still connects.
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
				ctx.renderer->draw_tile(
					Vector2D{ col, row },
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
#ifndef __EMSCRIPTEN__
				TileRef decorRef = ctx.decorEditor->get_override(Vector2D{ col, row });
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

	// Draw a random item from the registry for this level
	assert(ctx.levelManager && "Map::add_item: called without a levelManager");
	ItemFactory::spawn_random_item(pos, ctx.levelManager->get_dungeon_level(), ctx);
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
			set_tile(Vector2D{ tileX, tileY }, TileType::FLOOR);
		}
	}
}

void Map::dig_corridor(Vector2D begin, Vector2D end)
{
	const int beginCol = begin.x;
	const int beginRow = begin.y;
	const int endCol = end.x;
	const int endRow = end.y;

	const bool horizontal_first = mapRng.roll(0, 1) == 1;

	// Only WALL tiles are carved, so a corridor crossing another room's box leaves its floor alone.
	auto dig_one = [&](Vector2D pos)
	{
		if (!in_bounds(pos))
		{
			return;
		}

		if (get_tile_type(pos) == TileType::WALL)
		{
			set_tile(pos, TileType::CORRIDOR);
		}
	};

	if (horizontal_first)
	{
		for (int col = std::min(beginCol, endCol); col <= std::max(beginCol, endCol); ++col)
		{
			dig_one(Vector2D{ col, beginRow });
		}
		for (int row = std::min(beginRow, endRow); row <= std::max(beginRow, endRow); ++row)
		{
			if (row == beginRow)
			{
				continue;
			}
			dig_one(Vector2D{ endCol, row });
		}
	}
	else
	{
		for (int row = std::min(beginRow, endRow); row <= std::max(beginRow, endRow); ++row)
		{
			dig_one(Vector2D{ beginCol, row });
		}
		for (int col = std::min(beginCol, endCol); col <= std::max(beginCol, endCol); ++col)
		{
			if (col == beginCol)
			{
				continue;
			}
			dig_one(Vector2D{ col, endRow });
		}
	}
}

int Map::exit_column(const DungeonRoom& room, int interiorRow) const
{
	// Walk outwards from the centre column to the nearest one whose cell on this row is floor.
	const int centre = room.center_col();
	for (int offset = 0; offset <= room.width; ++offset)
	{
		for (const int column : { centre - offset, centre + offset })
		{
			if (column < room.col || column > room.col_end())
			{
				continue;
			}
			if (get_tile_type(Vector2D{ column, interiorRow }) != TileType::WALL)
			{
				return column;
			}
		}
	}
	// Nothing on this edge is floor. The centre keeps the old behaviour and leaves the
	// connectivity assert in Map::init to name it rather than hiding it here.
	return centre;
}

int Map::exit_row(const DungeonRoom& room, int interiorColumn) const
{
	const int centre = room.center_row();
	for (int offset = 0; offset <= room.height; ++offset)
	{
		for (const int row : { centre - offset, centre + offset })
		{
			if (row < room.row || row > room.row_end())
			{
				continue;
			}
			if (get_tile_type(Vector2D{ interiorColumn, row }) != TileType::WALL)
			{
				return row;
			}
		}
	}
	return centre;
}

bool Map::room_layout_is_sound(const DungeonRoom& room) const
{
	// A corridor leaves by a cell on one of the four edge lines, so each line needs at
	// least one square of floor or the room cannot be entered from that side at all.
	auto edge_has_floor = [this](Vector2D from, Vector2D step, int cells)
	{
		for (int taken = 0; taken < cells; ++taken)
		{
			if (get_tile_type(Vector2D{ from.x + step.x * taken, from.y + step.y * taken }) != TileType::WALL)
			{
				return true;
			}
		}
		return false;
	};

	const bool everyEdgeIsOpen =
		edge_has_floor(Vector2D{ room.col, room.row }, Vector2D{ 1, 0 }, room.width) &&
		edge_has_floor(Vector2D{ room.col, room.row_end() }, Vector2D{ 1, 0 }, room.width) &&
		edge_has_floor(Vector2D{ room.col, room.row }, Vector2D{ 0, 1 }, room.height) &&
		edge_has_floor(Vector2D{ room.col_end(), room.row }, Vector2D{ 0, 1 }, room.height);

	return everyEdgeIsOpen && room_interior_is_one_piece(room);
}

bool Map::room_interior_is_one_piece(const DungeonRoom& room) const
{
	// A corridor crossing the box is walled off from the room, so only its floor and water count.
	auto belongs_to_room = [this](Vector2D pos)
	{
		const TileType type = get_tile_type(pos);
		return type == TileType::FLOOR || type == TileType::WATER;
	};
	std::vector<bool> reached(tiles.size(), false);
	std::vector<Vector2D> pending;
	int floorCells = 0;

	for (int row = room.row; row <= room.row_end(); ++row)
	{
		for (int column = room.col; column <= room.col_end(); ++column)
		{
			if (belongs_to_room(Vector2D{ column, row }))
			{
				++floorCells;
				if (pending.empty())
				{
					pending.push_back(Vector2D{ column, row });
					reached[get_index(pending.front())] = true;
				}
			}
		}
	}

	if (floorCells == 0)
	{
		return true;
	}

	int reachedCells = 0;
	while (!pending.empty())
	{
		const Vector2D here = pending.back();
		pending.pop_back();
		++reachedCells;

		const Vector2D steps[4] = {
			Vector2D{ here.x + 1, here.y }, Vector2D{ here.x - 1, here.y }, Vector2D{ here.x, here.y + 1 }, Vector2D{ here.x, here.y - 1 }
		};
		for (const Vector2D& next : steps)
		{
			if (!room.contains(next.x, next.y) || reached[get_index(next)])
			{
				continue;
			}
			if (!belongs_to_room(next))
			{
				continue;
			}
			reached[get_index(next)] = true;
			pending.push_back(next);
		}
	}

	return reachedCells == floorCells;
}

void Map::set_door(Vector2D position)
{
	set_tile(position, TileType::CLOSED_DOOR);

	size_t tileIndex = get_index(position);
	if (tileIndex < tiles.size())
	{
		tiles[tileIndex].doorState = DoorState::CLOSED_UNLOCKED;
	}
}

void Map::set_tile(Vector2D pos, TileType newType)
{
	if (!in_bounds(pos))
	{
		return;
	}
	const size_t idx = get_index(pos);
	tiles.at(idx).type = newType;

	// Keep fovMap in sync so is_wall and is_unobstructed see the change immediately.
	fovMap->set_properties(pos.x, pos.y, fov_properties_for(newType));
}

void Map::apply_room_shape(const DungeonRoom& room)
{
	// Before dig() the centre is still wall, and every wall_back below would change nothing.
	assert(!is_wall(Vector2D{ room.col + room.width / 2, room.row + room.height / 2 })
		&& "apply_room_shape: the room has not been dug yet, so shaping it does nothing");

	auto wall_back = [&](int col, int row)
	{
		set_tile(Vector2D{ col, row }, TileType::WALL);
	};

	switch (room.shape)
	{
	case RoomShape::RECT:
	{
		// Full bounding box already dug - nothing to remove.
		break;
	}

	case RoomShape::L_SHAPE:
	{
		// Remove one W/3 x H/3 corner quadrant.
		// variant 0=top-right, 1=top-left, 2=bottom-right, 3=bottom-left.
		const int cutW = room.width / 3;
		const int cutH = room.height / 3;

		int xStart{};
		int xEnd{};
		int yStart{};
		int yEnd{};

		switch (room.shapeVariant % 4)
		{
		case 0: // top-right
		{
			xStart = room.col_end() - cutW + 1;
			xEnd = room.col_end();
			yStart = room.row;
			yEnd = room.row + cutH - 1;
			break;
		}
		case 1: // top-left
		{
			xStart = room.col;
			xEnd = room.col + cutW - 1;
			yStart = room.row;
			yEnd = room.row + cutH - 1;
			break;
		}
		case 2: // bottom-right
		{
			xStart = room.col_end() - cutW + 1;
			xEnd = room.col_end();
			yStart = room.row_end() - cutH + 1;
			yEnd = room.row_end();
			break;
		}
		case 3: // bottom-left
		{
			xStart = room.col;
			xEnd = room.col + cutW - 1;
			yStart = room.row_end() - cutH + 1;
			yEnd = room.row_end();
			break;
		}
		}

		for (int row = yStart; row <= yEnd; ++row)
		{
			for (int col = xStart; col <= xEnd; ++col)
			{
				wall_back(col, row);
			}
		}
		break;
	}

	case RoomShape::CHAMFERED:
	{
		// Cut the single corner cell at each of the four corners.
		wall_back(room.col, room.row);
		wall_back(room.col_end(), room.row);
		wall_back(room.col, room.row_end());
		wall_back(room.col_end(), room.row_end());
		break;
	}

	case RoomShape::CROSS:
	{
		// Keep center row band (H/3 thick) and center column band (W/3 wide).
		// Wall back every cell that falls outside both bands.
		const int armThicknessH = std::max(2, room.height / 3);
		const int armThicknessW = std::max(2, room.width / 3);
		const int rowBandStart = room.row + (room.height - armThicknessH) / 2;
		const int rowBandEnd = rowBandStart + armThicknessH - 1;
		const int colBandStart = room.col + (room.width - armThicknessW) / 2;
		const int colBandEnd = colBandStart + armThicknessW - 1;

		for (int row = room.row; row <= room.row_end(); ++row)
		{
			for (int col = room.col; col <= room.col_end(); ++col)
			{
				const bool inRowBand = (row >= rowBandStart && row <= rowBandEnd);
				const bool inColBand = (col >= colBandStart && col <= colBandEnd);
				if (!inRowBand && !inColBand)
				{
					wall_back(col, row);
				}
			}
		}
		break;
	}

	case RoomShape::PILLARED:
	{
		// Four 1x1 pillars, two cells in from each corner, only where the room keeps floor around them.
		constexpr int pillarOffset = 2;
		if (room.width >= 8 && room.height >= 6)
		{
			wall_back(room.col + pillarOffset, room.row + pillarOffset);
			wall_back(room.col_end() - pillarOffset, room.row + pillarOffset);
			wall_back(room.col + pillarOffset, room.row_end() - pillarOffset);
			wall_back(room.col_end() - pillarOffset, room.row_end() - pillarOffset);
		}
		break;
	}
	}
}

void Map::create_room(const DungeonRoom& room, bool first, GameContext& ctx)
{
	assert(ctx.rooms && "Map::create_room: called without rooms");
	ctx.rooms->push_back(room);

	dig(Vector2D{ room.col, room.row }, Vector2D{ room.col_end(), room.row_end() });

	// Carve shape: wall back cells excluded by the chosen RoomShape.
	// Must run after dig() and before any spawning so all tile checks see correct state.
	apply_room_shape(room);

	// Stamp prefab decorations before water so water can see occupied tiles.
	assert(ctx.decorEditor && "Map::create_room: called without a decorEditor");
	assert(ctx.prefabLibrary && "Map::create_room: called without a prefabLibrary");
	ctx.prefabLibrary->apply_to_room(room, *ctx.decorEditor, *this);

	// Richest layout first, giving a layer up only when it fails: shape over prefab, prefab alone, plain box.
	// A dug rectangle is whole with floor on every edge, so the last always succeeds.
	if (!room_layout_is_sound(room))
	{
		dig(Vector2D{ room.col, room.row }, Vector2D{ room.col_end(), room.row_end() });
		ctx.prefabLibrary->apply_to_room(room, *ctx.decorEditor, *this);

		if (!room_layout_is_sound(room))
		{
			dig(Vector2D{ room.col, room.row }, Vector2D{ room.col_end(), room.row_end() });
		}
	}

	// Register a blocking Decoration for every stamped tile.
	assert(ctx.decorations && "Map::create_room: called without decorations");
	for (int row = room.row; row <= room.row_end(); ++row)
	{
		for (int col = room.col; col <= room.col_end(); ++col)
		{
			TileRef overrideTile = ctx.decorEditor->get_override(Vector2D{ col, row });
			if (!overrideTile.is_valid())
			{
				continue;
			}
			auto decoration = std::make_unique<Decoration>();
			decoration->position = Vector2D{ col, row };
			decoration->tile = overrideTile;
			decoration->name = "decoration";
			decoration->hp = 3;
			decoration->blocks_movement = true;
			decoration->lootTableKey = "";
			decoration->isBroken = false;
			ctx.decorations->push_back(std::move(decoration));
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
	assert(ctx.traps && "Map::spawn_traps called without a trap container");

	// ~30% of rooms get 0-2 random traps (was 10%)
	if (ctx.dice->d10() > 3)
	{
		return; // Room has no traps
	}

	// Room gets 1-2 random traps
	int trapCount = ctx.dice->roll(1, 2);

	for (int trapIndex = 0; trapIndex < trapCount; ++trapIndex)
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

			// Must be a floor tile (walkable, no features/items)
			if (tileType == TileType::FLOOR)
			{
				foundSpot = true;
				break;
			}
		}

		if (!foundSpot)
		{
			continue; // Couldn't find a valid spot for this trap
		}

		// Create trap (randomly choose type)
		TrapType trapType;
		int trapChoice = ctx.dice->roll(1, 3);
		switch (trapChoice)
		{
		case 1:
		{
			trapType = TrapType::PIT;
			break;
		}
		case 2:
		{
			trapType = TrapType::DART;
			break;
		}
		case 3:
		{
			trapType = TrapType::ARROW;
			break;
		}
		default:
		{
			trapType = TrapType::PIT;
		}
		}

		auto trap = std::make_unique<Trap>(trapPos, trapType, *ctx.tileConfig);
		ctx.traps->push_back(std::move(trap));
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
				// Only on carved floor: a shaped room leaves wall inside its box, and water there is sealed in rock.
				if (get_tile_type(waterPos) != TileType::FLOOR)
				{
					continue;
				}
				// Never place water on a decorated tile.
				assert(ctx.decorEditor && "Map::spawn_water: called without a decorEditor");
				if (ctx.decorEditor->get_override(waterPos).is_valid())
				{
					continue;
				}
				// CRITICAL FIX: Check if water would block entrances using pattern matching
				if (!would_water_block_entrance(waterPos, ctx))
				{
					set_tile(waterPos, TileType::WATER);
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

	// Check vertical corridor pattern: wall above and below (W/F/W)
	if (isWallOrOOB({ waterPos.x, waterPos.y - 1 }) && isWallOrOOB({ waterPos.x, waterPos.y + 1 }))
	{
		return true; // Would block vertical passage
	}

	// ENTRANCE PATTERN 3: near a room edge, where a door may later be placed

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
	assert(ctx.rooms && "Map::would_water_block_entrance: called without rooms");
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

	// If none of the blocking patterns match, water is safe to place
	return false;
}

void Map::spawn_items(const DungeonRoom& room, GameContext& ctx)
{
	assert(ctx.dice && "Map::spawn_items: called without dice");
	const int numItems = ctx.dice->roll(0, MAX_ROOM_ITEMS);
	for (int itemIndex = 0; itemIndex < numItems; itemIndex++)
	{
		Vector2D itemPos{ ctx.dice->roll(room.col, room.col_end()), ctx.dice->roll(room.row, room.row_end()) };
		constexpr int MAX_ITEM_TRIES = 20;
		int itemTries = 0;
		while (itemTries < MAX_ITEM_TRIES &&
			!can_place_at(itemPos, ctx))
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
	assert(ctx.decorations && "Map::spawn_barrels: called without decorations");
	assert(ctx.tileConfig && "Map::spawn_barrels: called without a tileConfig");

	constexpr int MAX_ROOM_BARRELS = 2;
	const int count = mapRng.roll(0, MAX_ROOM_BARRELS);
	const TileRef barrel_tile = ctx.tileConfig->get("TILE_BARREL");

	for (int barrelIndex = 0; barrelIndex < count; ++barrelIndex)
	{
		Vector2D pos{ mapRng.roll(room.col, room.col_end()), mapRng.roll(room.row, room.row_end()) };
		constexpr int MAX_TRIES = 20;
		int tries = 0;
		while (tries < MAX_TRIES && !can_place_at(pos, ctx))
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
	assert(ctx.player() && "Map::spawn_player called without player");
	assert(ctx.dice && "Map::spawn_player called without dice");

	Vector2D pos{ ctx.dice->roll(room.col, room.col_end()), ctx.dice->roll(room.row, room.row_end()) };
	constexpr int MAX_PLAYER_TRIES = 50;
	int playerTries = 0;
	while (playerTries < MAX_PLAYER_TRIES &&
		!can_place_at(pos, ctx))
	{
		pos.x = ctx.dice->roll(room.col, room.col_end());
		pos.y = ctx.dice->roll(room.row, room.row_end());
		++playerTries;
	}
	ctx.player()->position = pos;
}

void Map::place_stairs(GameContext& ctx)
{
	assert(ctx.stairs && "Map::place_stairs called without stairs");
	assert(ctx.rooms && !ctx.rooms->empty() && "Map::place_stairs called without rooms");
	assert(ctx.dice && "Map::place_stairs called without dice");

	// BFS from room[0] to find the deepest room - forces player to explore
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
		if (can_place_at(stairsPos, ctx))
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
	assert(ctx.stairs && "Map::is_stairs: called without stairs");
	return ctx.stairs->position == pos;
}

bool Map::is_unobstructed(Vector2D pos, const GameContext& ctx) const noexcept
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

bool Map::can_place_at(Vector2D pos, const GameContext& ctx) const noexcept
{
	return is_unobstructed(pos, ctx) && !is_water(pos);
}

void Map::add_monster(Vector2D pos, GameContext& ctx) const
{
	// Use the monster factory to create a monster appropriate for the current dungeon level
	assert(ctx.levelManager && "Map::add_monster: called without a levelManager");
	MonsterFactory::spawn_random_monster(pos, ctx.levelManager->get_dungeon_level(), ctx);

	// Log the spawn for debugging
	Creature* monster = get_actor(pos, ctx);
	assert(ctx.messageSystem && "Map::add_monster: called without a messageSystem");
	if (monster)
	{
		ctx.messageSystem->log("Spawned " + monster->actorData.name + " at level " + std::to_string(ctx.levelManager->get_dungeon_level()));
	}
}

// getActor returns the actor at the given coordinates or `nullptr` if there's none
Creature* Map::get_actor(Vector2D pos, const GameContext& ctx) const noexcept
{
	assert(ctx.creatures && "Map::get_actor: called without creatures");

	// Check player position first
	if (ctx.player() && ctx.player()->position == pos)
	{
		return ctx.player();
	}

	// Check all creatures (excluding dead ones - they shouldn't block)
	for (const auto& actor : *ctx.creatures)
	{
		assert(actor && "Map::get_actor: creatures holds a null entry");
		if (actor->position == pos)
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
	assert(ctx.creatures && "Map::regenerate: called without creatures");
	ctx.creatures->clear();
	assert(ctx.floorInventory && "Map::regenerate: called without a floorInventory");
	ctx.floorInventory->items.clear();
	assert(ctx.rooms && "Map::regenerate: called without rooms");
	ctx.rooms->clear(); // we clear the room coordinates
	assert(ctx.traps && "Map::regenerate: called without traps");
	ctx.traps->clear();
	assert(ctx.spellTiles && "Map::regenerate: called without spellTiles");
	ctx.spellTiles->clear();
	assert(ctx.decorations && "Map::regenerate: called without decorations");
	ctx.decorations->clear();

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

std::vector<Vector2D> Map::bresenham_line(Vector2D from, Vector2D to)
{
	std::vector<Vector2D> path;

	int currentCol = from.x;
	int currentRow = from.y;
	const int endCol = to.x;
	const int endRow = to.y;

	const int deltaCol = std::abs(endCol - currentCol);
	const int deltaRow = std::abs(endRow - currentRow);
	const int stepCol = (currentCol < endCol) ? 1 : -1;
	const int stepRow = (currentRow < endRow) ? 1 : -1;
	int err = deltaCol - deltaRow;

	while (currentCol != endCol || currentRow != endRow)
	{
		int doubledError = 2 * err;
		if (doubledError > -deltaRow)
		{
			err -= deltaRow;
			currentCol += stepCol;
		}
		if (doubledError < deltaCol)
		{
			err += deltaCol;
			currentRow += stepRow;
		}
		path.push_back(Vector2D{ currentCol, currentRow });
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
	{
		return false;
	}

	TileType tileType = get_tile_type(pos);
	return tileType == TileType::CLOSED_DOOR || tileType == TileType::OPEN_DOOR;
}

bool Map::is_open_door(Vector2D pos) const noexcept
{
	if (pos.y < 0 || pos.y >= mapHeight || pos.x < 0 || pos.x >= mapWidth)
	{
		return false;
	}
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

	set_tile(pos, TileType::OPEN_DOOR);

	// Clear door state when opening
	size_t tileIndex = get_index(pos);
	if (tileIndex < tiles.size())
	{
		tiles[tileIndex].doorState = DoorState::OPEN;
	}

	if (ctx.player() && ctx.player()->get_tile_distance(pos) <= Vision::sight_radius(*ctx.player()))
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
	assert(ctx.floorInventory && "Map::close_door: called without a floorInventory");
	[[maybe_unused]] auto is_null = [](const auto& entry)
	{
		return !entry;
	};
	assert(std::ranges::none_of(ctx.floorInventory->items, is_null) && "Map::close_door: the floor holds a null item");
	auto has_item_at_pos = [&pos](const std::unique_ptr<Item>& item)
	{
		return item->position == pos;
	};
	if (std::ranges::any_of(ctx.floorInventory->items, has_item_at_pos))
	{
		return false;
	}

	// Change the tile type to DOOR (closed)
	set_tile(pos, TileType::CLOSED_DOOR);

	// New doors are unlocked by default
	size_t tileIndex = get_index(pos);
	if (tileIndex < tiles.size())
	{
		tiles[tileIndex].doorState = DoorState::CLOSED_UNLOCKED;
	}

	if (ctx.player() && ctx.player()->get_tile_distance(pos) <= Vision::sight_radius(*ctx.player()))
	{
		compute_fov(ctx);
	}

	return true;
}

// Locks a closed door, the counterpart to unlock_door. Refuses anything that is
// not a closed door, and refuses a door already locked, so the return says
// whether this call is what locked it.
//
// Example:
//   map.set_door(Vector2D{ 5, 7 });   // placed closed and unlocked
//   map.lock_door(Vector2D{ 5, 7 });  // -> true
//   map.lock_door(Vector2D{ 5, 7 });  // -> false, already locked
//   map.lock_door(Vector2D{ 8, 7 });  // -> false, a stretch of floor
bool Map::lock_door(Vector2D pos)
{
	// An open door, a wall or floor has no lock to turn.
	if (!is_door(pos) || get_tile_type(pos) != TileType::CLOSED_DOOR)
	{
		return false;
	}

	size_t tileIndex = get_index(pos);
	if (tileIndex >= tiles.size())
	{
		return false;
	}

	// Already locked, so this call changed nothing.
	if (tiles[tileIndex].doorState == DoorState::CLOSED_LOCKED)
	{
		return false;
	}

	tiles[tileIndex].doorState = DoorState::CLOSED_LOCKED;
	return true;
}

bool Map::unlock_door(Vector2D pos)
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
	assert(ctx.rooms && "Map::open_all_room_doors: called without rooms");

	for (const auto& room : *ctx.rooms)
	{
		bool belongsToRoom = false;
		for (Vector2D dir : { DIR_N, DIR_S, DIR_E, DIR_W })
		{
			Vector2D neighbor = doorPos + dir;
			if (room.contains(neighbor.x, neighbor.y))
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
		for (int row = room.top_wall(); row <= room.bottom_wall(); ++row)
		{
			for (int col = room.left_wall(); col <= room.right_wall(); ++col)
			{
				Vector2D pos{ col, row };
				if (!in_bounds(pos))
				{
					continue;
				}
				if (!is_door_locked(pos))
				{
					continue;
				}
				// Doors are locked only on a room's wall ring, so the floor this scan crosses
				// never holds one.
				assert(!room.contains(col, row) && "open_all_room_doors: a locked door inside a room's floor");

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
					unlock_door(pos);
					open_door(pos, ctx);
				}
			}
		}
		return;
	}

	// Fallback: no room found, open only this door.
	unlock_door(doorPos);
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
	assert(ctx.floorInventory && "Map::place_amulet: called without a floorInventory");

	// Only place the amulet on the final level
	assert(ctx.levelManager && "Map::place_amulet called without levelManager");
	assert(ctx.player() && "Map::place_amulet called without player");
	assert(ctx.rooms && "Map::place_amulet called without rooms");
	assert(ctx.dice && "Map::place_amulet called without dice");

	if (ctx.levelManager->get_dungeon_level() == FINAL_DUNGEON_LEVEL)
	{
		// Choose a random room for the amulet
		const int index = ctx.dice->roll(0, static_cast<int>(ctx.rooms->size()) - 1);
		const DungeonRoom& room = ctx.rooms->at(index);

		// Somewhere reachable without water: the amulet is placed once, and a character who
		// later loses the means to swim must still be able to win.
		Vector2D amuletPos{ ctx.dice->roll(room.col, room.col_end()), ctx.dice->roll(room.row, room.row_end()) };
		while (!can_place_at(amuletPos, ctx) || is_stairs(amuletPos, ctx))
		{
			amuletPos.x = ctx.dice->roll(room.col, room.col_end());
			amuletPos.y = ctx.dice->roll(room.row, room.row_end());
		}

		// Create and place the amulet
		InventoryOperations::add_item(*ctx.floorInventory, ItemCreator::create("amulet_of_yendor", amuletPos, ctx));

		// Log the placement (debug info)
		assert(ctx.messageSystem && "Map::place_amulet: called without a messageSystem");
		ctx.messageSystem->log("Placed Amulet of Yendor at " + std::to_string(amuletPos.x) + "," + std::to_string(amuletPos.y));

		// Add a hint message
		ctx.messageSystem->message(ColorPairId::RED_YELLOW, "You sense a powerful artifact somewhere on this level...", MessageCompletion::FINISHED);
	}
}

void Map::post_process_doors()
{
	auto isRoom = [this](Vector2D pos)
	{
		return in_bounds(pos) && (get_tile_type(pos) == TileType::FLOOR || get_tile_type(pos) == TileType::WATER);
	};
	auto isWall = [this](Vector2D pos)
	{
		return in_bounds(pos) && get_tile_type(pos) == TileType::WALL;
	};

	for (int row = 0; row < mapHeight; ++row)
	{
		for (int col = 0; col < mapWidth; ++col)
		{
			Vector2D pos{ col, row };
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
					Vector2D upLeft = { pos.x - 1, pos.y - 1 };
					Vector2D up = { pos.x, pos.y - 1 };
					Vector2D upRight = { pos.x + 1, pos.y - 1 };
					Vector2D left = { pos.x - 1, pos.y };
					Vector2D right = { pos.x + 1, pos.y };
					Vector2D downLeft = { pos.x - 1, pos.y + 1 };
					Vector2D down = { pos.x, pos.y + 1 };
					Vector2D downRight = { pos.x + 1, pos.y + 1 };

					// Exclude pattern RRR/WCC/WWW (and rotations)
					bool shouldExclude = false;

					// Original: RRR/WCC/WWW
					if (isRoom(upLeft) && isRoom(up) && isRoom(upRight) &&
						isWall(left) && get_tile_type(right) == TileType::CORRIDOR &&
						isWall(downLeft) && isWall(down) && isWall(downRight))
					{
						shouldExclude = true;
					}

					// 90 degree rotation: WWR/CWR/CWR
					if (isWall(upLeft) && isWall(up) && isRoom(upRight) &&
						get_tile_type(left) == TileType::CORRIDOR && isRoom(right) &&
						get_tile_type(downLeft) == TileType::CORRIDOR && isWall(down) && isRoom(downRight))
					{
						shouldExclude = true;
					}

					// 180 degree rotation: WWW/CCW/RRR
					if (isWall(upLeft) && isWall(up) && isWall(upRight) &&
						get_tile_type(left) == TileType::CORRIDOR && isWall(right) &&
						isRoom(downLeft) && isRoom(down) && isRoom(downRight))
					{
						shouldExclude = true;
					}

					// 270 degree rotation: RWC/RWC/RWW
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
						Vector2D upPos = { pos.x, pos.y - 1 };
						set_door(upPos);
					}
					// Check for W.w/CDW/WWW pattern (move door UP) - where . = water or corridor
					else if (isWall(upLeft) &&
						(get_tile_type(up) == TileType::WATER || get_tile_type(up) == TileType::CORRIDOR) &&
						get_tile_type(upRight) == TileType::WATER &&
						get_tile_type(left) == TileType::CORRIDOR && isWall(right) &&
						isWall(downLeft) && isWall(down) && isWall(downRight))
					{
						// Move door UP
						Vector2D upPos = { pos.x, pos.y - 1 };
						set_door(upPos);
					}
					// Check for Z-pattern: WRR/CCW/WWW (move door left)
					else if (isWall(upLeft) && isRoom(up) && isRoom(upRight) &&
						get_tile_type(left) == TileType::CORRIDOR && isWall(right) &&
						isWall(downLeft) && isWall(down) && isWall(downRight))
					{
						// Move door LEFT
						Vector2D leftPos = { pos.x - 1, pos.y };
						set_door(leftPos);
					}
					else
					{
						// All other doors (including WRR/CCR/WRR pattern)
						set_door(pos);
					}
				}
			}
		}
	}
}

// ---- place_from_graph: render dungeon graph (rooms + edges) to tiles ----
// Idempotent: can regenerate from the same room graph without data loss.
// Separates graph construction from tile rendering.

void Map::place_from_graph(
	const std::vector<DungeonRoom>& rooms,
	GameContext& ctx)
{
	assert(ctx.messageSystem && "Map::place_from_graph: called without a messageSystem");
	ctx.messageSystem->log("place_from_graph: Starting with " + std::to_string(rooms.size()) + " rooms");

	// Step 1: Create all rooms (dig them, spawn water/items/player)
	for (int roomIndex = 0; roomIndex < static_cast<int>(rooms.size()); ++roomIndex)
	{
		create_room(rooms[roomIndex], roomIndex == 0, ctx);
	}

	assert(ctx.messageSystem && "Map::place_from_graph: no messageSystem once the rooms were created");
	ctx.messageSystem->log("place_from_graph: Rooms created, now connecting with corridors");

	// Step 2: Connect adjacent rooms with corridors (from graph edges)
	std::set<std::pair<int, int>> dug_edges;

	for (int a_idx = 0; a_idx < static_cast<int>(rooms.size()); ++a_idx)
	{
		const DungeonRoom& room = rooms[a_idx];

		for (int b_idx : room.adjacentRoomIndices)
		{
			// Avoid digging the same corridor twice
			std::pair<int, int> edge_forward(std::min(a_idx, b_idx), std::max(a_idx, b_idx));
			if (dug_edges.count(edge_forward))
			{
				continue;
			}
			dug_edges.insert(edge_forward);

			const DungeonRoom& adjacent = rooms[b_idx];

			// Determine relative positions and dig L-shape corridor
			const bool a_below = room.top_wall() > adjacent.bottom_wall();
			const bool a_right = room.left_wall() > adjacent.right_wall();
			const bool a_above = room.bottom_wall() < adjacent.top_wall();
			const bool a_left = room.right_wall() < adjacent.left_wall();

			// Each room is left by a cell its floor reaches; a corridor aimed at a walled centre opens onto rock.
			if (a_below)
			{
				// room is below adjacent -- corridor rises from room, crosses horizontally, drops to adjacent
				const int mid = (adjacent.bottom_wall() + room.top_wall()) / 2;
				const int aCol = exit_column(room, room.row);
				const int bCol = exit_column(adjacent, adjacent.row_end());
				dig_corridor(Vector2D{ aCol, room.top_wall() }, Vector2D{ aCol, mid });
				dig_corridor(Vector2D{ aCol, mid }, Vector2D{ bCol, mid });
				dig_corridor(Vector2D{ bCol, mid }, Vector2D{ bCol, adjacent.bottom_wall() });
			}
			else if (a_right)
			{
				// room is to the right of adjacent
				const int mid = (adjacent.right_wall() + room.left_wall()) / 2;
				const int aRow = exit_row(room, room.col);
				const int bRow = exit_row(adjacent, adjacent.col_end());
				dig_corridor(Vector2D{ room.left_wall(), aRow }, Vector2D{ mid, aRow });
				dig_corridor(Vector2D{ mid, aRow }, Vector2D{ mid, bRow });
				dig_corridor(Vector2D{ mid, bRow }, Vector2D{ adjacent.right_wall(), bRow });
			}
			else if (a_above)
			{
				// room is above adjacent
				const int mid = (room.bottom_wall() + adjacent.top_wall()) / 2;
				const int aCol = exit_column(room, room.row_end());
				const int bCol = exit_column(adjacent, adjacent.row);
				dig_corridor(Vector2D{ aCol, room.bottom_wall() }, Vector2D{ aCol, mid });
				dig_corridor(Vector2D{ aCol, mid }, Vector2D{ bCol, mid });
				dig_corridor(Vector2D{ bCol, mid }, Vector2D{ bCol, adjacent.top_wall() });
			}
			else if (a_left)
			{
				// room is to the left of adjacent
				const int mid = (room.right_wall() + adjacent.left_wall()) / 2;
				const int aRow = exit_row(room, room.col_end());
				const int bRow = exit_row(adjacent, adjacent.col);
				dig_corridor(Vector2D{ room.right_wall(), aRow }, Vector2D{ mid, aRow });
				dig_corridor(Vector2D{ mid, aRow }, Vector2D{ mid, bRow });
				dig_corridor(Vector2D{ mid, bRow }, Vector2D{ adjacent.left_wall(), bRow });
			}
			else
			{
				// Rooms are adjacent or overlapping -- direct L-shape between centres.
				dig_corridor(
					Vector2D{ room.center_col(), room.center_row() },
					Vector2D{ adjacent.center_col(), adjacent.center_row() });
			}
		}
	}

	assert(ctx.messageSystem && "Map::place_from_graph: no messageSystem once the corridors were dug");
	ctx.messageSystem->log("place_from_graph: Finished. Dug " + std::to_string(dug_edges.size()) + " edges");
}

// end of file: Map.cpp
