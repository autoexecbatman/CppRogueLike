#pragma once

#include <array>
#include <memory>
#include <vector>

#include "Decoration.h"
#include "DungeonRoom.h"
#include "FovMap.h"
#include "Persistent.h"
#include "RandomDice.h"
#include "TileType.h"

// Forward declaration
struct GameContext;
class Creature;

inline constexpr int DEFAULT_MAP_WIDTH = 120;
inline constexpr int DEFAULT_MAP_HEIGHT = 80;

inline int get_map_width()
{
	return DEFAULT_MAP_WIDTH;
}

inline int get_map_height()
{
	return DEFAULT_MAP_HEIGHT;
}

inline constexpr int FOV_RADIUS = 4;

inline constexpr int ROOM_HORIZONTAL_MAX_SIZE = 14;
inline constexpr int ROOM_VERTICAL_MAX_SIZE = 9;
inline constexpr int ROOM_MIN_SIZE = 6;
inline constexpr int MAX_ROOM_ITEMS = 4;
inline constexpr int MAX_MONSTERS = 6;
inline constexpr int FINAL_DUNGEON_LEVEL = 10;

//==Tile==
// A tile of the map
// checks if the player has seen this tile
// it is used for field of view algorithm (a 2D array) (see Map.h) (see Map.cpp) (see Map::computeFov())

enum class DoorState
{
	OPEN,
	CLOSED_UNLOCKED,
	CLOSED_LOCKED,
	// Locked, and the player has used the one attempt Table 1 allows at forcing it.
	CLOSED_LOCKED_FORCE_FAILED
};

struct Tile
{
	Vector2D position{};
	TileType type{};
	bool explored{};
	DoorState doorState{ DoorState::OPEN };
	// The player's experience level when they last failed to pick this lock, or 0 for never.
	int lockPickFailedAtLevel{ 0 };

	Tile(Vector2D pos, TileType type)
		: position(pos), type(type), explored(false), doorState(DoorState::OPEN) {}
};

//==Map==
// a class for the game map
// the map is a 2d array of tiles
class Map : public Persistent
{
private:
	int mapWidth{};
	int mapHeight{};
	static constexpr std::array<Vector2D, 8> DIRS = { DIR_N, DIR_NE, DIR_E, DIR_SE, DIR_S, DIR_SW, DIR_W, DIR_NW };

	Vector2D get_map_size() const noexcept
	{
		Vector2D size{ 0, 0 };
		size.x = mapWidth;
		size.y = mapHeight;
		return size;
	}

	bool in_bounds(Vector2D pos) const noexcept;

	void place_stairs(GameContext& ctx);
	bool is_stairs(Vector2D pos, GameContext& ctx) const;
	void spawn_water(const DungeonRoom& room, GameContext& ctx);
	bool would_water_block_entrance(Vector2D waterPos, GameContext& ctx) const;
	void spawn_items(const DungeonRoom& room, GameContext& ctx);
	void spawn_barrels(const DungeonRoom& room, GameContext& ctx);
	void spawn_player(const DungeonRoom& room, GameContext& ctx);
	void generate_rooms(GameContext& ctx);

	bool is_floor(Vector2D pos) const noexcept { return get_tile_type(pos) == TileType::FLOOR; }

	void set_explored(Vector2D pos); // set the tile as explored
	void post_process_doors();

protected:
	std::unique_ptr<FovMap> fovMap{};
	long seed{ 0 };
	// Derived from seed, so a map that has not run init or load is the seed-0 map it reports.
	RandomDice mapRng{ static_cast<unsigned int>(seed) };
	friend class DungeonGenerator;
	void dig(Vector2D begin, Vector2D end);
	void dig_corridor(Vector2D begin, Vector2D end);
	void set_door(Vector2D position);
	void create_room(const DungeonRoom& room, bool first, GameContext& ctx);
	// Walls back cells inside the bounding box that the chosen RoomShape excludes.
	// Must be called after dig() and before any spawning. No-op for RECT.
	void apply_room_shape(const DungeonRoom& room);
	void spawn_traps(const DungeonRoom& room, GameContext& ctx);

public:
	Map(int mapWidth, int mapHeight);

	// Replaces the map with a saved one: its size, seed and every tile's type, whether it has
	// been seen, its door state and the level a pick of its lock failed at. Doors are restored
	// as saved, never placed again. Throws nlohmann's out_of_range naming any field a record
	// lacks, "doorState" and "lockPickFailedAtLevel" included.
	//
	// Example:
	//   fresh.load(saved);                      // saved by map.save; a door locked there is locked here
	//   fresh.is_door_locked(treasureDoor);     // -> true
	//   fresh.load(savedWithoutDoorState);      // throws "... key 'doorState' not found"
	void load(const json& source) override;
	// Writes the size, seed and every tile - type, seen, door state, failed pick - in the shape
	// load reads.
	//
	// Example:
	//   map.save(saved);   // saved["tiles"][0] -> { "position", "type", "explored", "doorState", "lockPickFailedAtLevel" }
	void save(json& output) override;

	// Initialize the tile grid to all-walls. No dungeon generation.
	// Useful as a test seam when unit tests need a blank map.
	void init_tiles();

	// Full dungeon initialization: tile grid + room generation + actor placement.
	void init(GameContext& ctx);
	bool is_in_fov(Vector2D pos) const noexcept;
	TileType get_tile_type(Vector2D pos) const noexcept;

	// Whether that tile is water. Public because drowning asks it from outside: a
	// creature's round has to know whether it is standing in the stuff.
	bool is_water(Vector2D pos) const noexcept;
	// Tells the player about the tile they are on, where it is worth remarking on.
	void describe_tile(TileType tileType, GameContext& ctx);
	// indicates whether this tile has already been seen by the player
	bool is_explored(Vector2D pos) const noexcept;

	// Whether nothing stands in the way at that tile: not a wall, not a shut door, no
	// creature and no decoration. It says nothing about what the terrain demands of
	// whoever crosses it, because it is asked where there is no mover - choosing where
	// to put an item down, expanding a search frontier.
	//
	//   map.is_unobstructed(pool, ctx);   // -> true, a pool obstructs nobody
	//
	// It is deliberately not named for walking: whether a creature may go there is
	// Pathfinding's question, and asking this one instead is how every monster in the
	// game once came to walk into water.
	bool is_unobstructed(Vector2D pos, const GameContext& ctx) const noexcept;
	// Whether generation may put something on that tile - a creature, an item, a barrel,
	// the stairs or the amulet: unobstructed, and dry, because the character a game starts
	// with cannot enter water, so whatever is put there is out of its reach.
	//
	//   map.can_place_at(floorTile, ctx); // -> true
	//   map.can_place_at(pool, ctx);      // -> false
	bool can_place_at(Vector2D pos, const GameContext& ctx) const noexcept;
	void add_monster(Vector2D pos, GameContext& ctx) const;
	void compute_fov(GameContext& ctx);
	void update();
	void render(const GameContext& ctx) const;
	void add_item(Vector2D pos, GameContext& ctx);
	// getActor returns the actor at the given coordinates or NULL if there's
	// none
	Creature* get_actor(Vector2D pos, const GameContext& ctx) const noexcept;
	std::vector<std::vector<Tile>> get_map() const noexcept;
	void reveal(); // reveal the map
	void regenerate(GameContext& ctx); // regenerate the map

	int get_width() const noexcept { return mapWidth; }

	int get_height() const noexcept { return mapHeight; }

	long get_seed() const noexcept { return seed; }

	bool is_in_bounds(Vector2D pos) const noexcept { return pos.x >= 0 && pos.x < mapWidth && pos.y >= 0 && pos.y < mapHeight; }

	static std::vector<Vector2D> bresenham_line(Vector2D from, Vector2D to);

	size_t get_index(Vector2D pos) const
	{
		if (in_bounds(pos))
		{
			return pos.y * mapWidth + pos.x;
		}
		else
		{
			throw std::out_of_range{ "Map::get_index() out of bounds" };
		}
	} // Note: Cannot be noexcept due to exception

	bool has_los(Vector2D from, Vector2D to) const noexcept;
	bool open_door(Vector2D pos, GameContext& ctx);
	bool close_door(Vector2D pos, GameContext& ctx);
	bool lock_door(Vector2D pos);
	bool unlock_door(Vector2D pos);
	bool is_door_locked(Vector2D pos) const noexcept;
	// Whether the player has already failed to force this locked door. Table 1 allows one
	// attempt per door, so a door that resisted once cannot be forced again; a key, a picked
	// lock or Knock still opens it.
	//
	// Example:
	//   map.has_resisted_forcing(lockedDoor);   // -> false, never tried
	//   map.mark_resisted_forcing(lockedDoor);
	//   map.has_resisted_forcing(lockedDoor);   // -> true, and is_door_locked is still true
	//   map.has_resisted_forcing(plainFloor);   // -> false
	bool has_resisted_forcing(Vector2D pos) const noexcept;
	// Records a failed attempt to force a locked door, which stays locked. The door must be
	// locked and not already marked: forcing is refused before a roll once it has failed.
	//
	// Example:
	//   map.mark_resisted_forcing(lockedDoor);   // has_resisted_forcing(lockedDoor) -> true
	void mark_resisted_forcing(Vector2D pos);
	// The experience level at which the player last failed to pick this lock, or 0 if they
	// never have. Open Locks allows one attempt per lock per level, so a pick is refused
	// until the player's level is higher than this. Off the map answers 0.
	//
	// Example:
	//   map.lock_pick_failed_at_level(lockedDoor);   // -> 0, never tried
	//   map.mark_lock_pick_failed(lockedDoor, 3);
	//   map.lock_pick_failed_at_level(lockedDoor);   // -> 3, and is_door_locked is still true
	//   map.lock_pick_failed_at_level(plainFloor);   // -> 0
	int lock_pick_failed_at_level(Vector2D pos) const noexcept;
	// Records a failed attempt to pick a locked door at the player's level. The door must be
	// locked and the level higher than any recorded, since a pick at a lower one is refused.
	//
	// Example:
	//   map.mark_lock_pick_failed(lockedDoor, 2);   // lock_pick_failed_at_level(lockedDoor) -> 2
	//   map.mark_lock_pick_failed(lockedDoor, 2);   // asserts: not higher than the recorded 2
	void mark_lock_pick_failed(Vector2D pos, int level);
	void open_all_room_doors(Vector2D doorPos, GameContext& ctx);
	void place_amulet(GameContext& ctx);
	// The first unbroken decoration standing on a tile, or null. A broken one stays in
	// the list until the game loop sweeps it, so it is skipped here rather than reported -
	// "nothing there" and "only wreckage there" are the same answer to this question. A
	// level built without a decoration list answers null rather than reaching through it.
	//
	// Example, a barrel on the tile:
	//   find_decoration_at(tile, ctx);   // -> the barrel
	//   barrel->isBroken = true;
	//   find_decoration_at(tile, ctx);   // -> nullptr
	Decoration* find_decoration_at(Vector2D pos, const GameContext& ctx) const noexcept;
	bool is_door(Vector2D pos) const noexcept;
	bool is_open_door(Vector2D pos) const noexcept;
	bool is_wall(Vector2D pos) const noexcept;

	// The column, or row, a corridor should leave `room` by along the given interior edge:
	// the one nearest the room's centre whose cell there is walkable. A prefab or a room
	// shape can wall the centre itself, and a corridor aimed there opens onto rock.
	//
	// Example:
	//   map.exit_column(room, room.row);        // -> the centre column, on an open room
	//   map.exit_column(notchedRoom, room.row); // -> a column beside it, past the notch
	int exit_column(const DungeonRoom& room, int interiorRow) const;
	int exit_row(const DungeonRoom& room, int interiorColumn) const;

	// Whether all of this room's floor is reachable from itself without leaving the room.
	// False when the shape and the prefab have carved across each other and split it.
	bool room_interior_is_one_piece(const DungeonRoom& room) const;

	// Whether this room can be used: its floor is one piece, and every edge line holds at
	// least one square of it, so a corridor can enter from any side. A room that fails
	// this was carved by the shape and the prefab together into something unenterable.
	bool room_layout_is_sound(const DungeonRoom& room) const;

	// How many non-wall tiles cannot be reached from start, walking four ways and
	// treating every door as a way through. Zero is the invariant generation must
	// leave: a pool or a room sealed inside rock is a defect. Returns -1 when start
	// is itself off the map or a wall, so a bad question cannot read as a clean map.
	//
	// Example:
	//   map.count_unreachable_tiles(entrance);        // -> 0 on a sound dungeon
	//   map.count_unreachable_tiles(Vector2D{0, 0});  // -> -1, the border is wall
	int count_unreachable_tiles(Vector2D start) const noexcept;

	// Whether every tile on the map's outer edge is wall. Generation must leave it so:
	// nothing can then stand on the edge, and every neighbour of a walkable tile is on
	// the map.
	//
	// Example:
	//   map.init_tiles();
	//   map.has_wall_border(); // -> true
	//   map.set_tile(Vector2D{ 0, 5 }, TileType::FLOOR);
	//   map.has_wall_border(); // -> false
	bool has_wall_border() const noexcept;
	void set_tile(Vector2D pos, TileType newType);
	void place_from_graph(
		const std::vector<DungeonRoom>& rooms,
		GameContext& ctx);

	std::vector<Tile> tiles{};
};
