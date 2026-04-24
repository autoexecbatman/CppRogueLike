#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "../Actor/Actor.h"
#include "../Factories/ItemFactory.h"
#include "../Factories/MonsterFactory.h"
#include "../Persistent/Persistent.h"
#include "../Random/RandomDice.h"
#include "Decoration.h"
#include "DungeonRoom.h"
#include "FovMap.h"

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

enum class TileType
{
	FLOOR,
	WALL,
	WATER,
	CLOSED_DOOR,
	OPEN_DOOR,
	CORRIDOR,
	// Add more as needed...
};

enum class DoorState
{
	OPEN,
	CLOSED_UNLOCKED,
	CLOSED_LOCKED
};

struct Tile
{
	Vector2D position;
	TileType type;
	bool explored;
	double cost;
	DoorState doorState { DoorState::OPEN };

	// overload the greater than operator
	bool operator>(const Tile& other) const { return cost > other.cost; }
	Tile(Vector2D pos, TileType type, double cost)
		: position(pos), type(type), explored(false), cost(cost), doorState(DoorState::OPEN) {}
};

//==Map==
// a class for the game map
// the map is a 2d array of tiles
class Map : public Persistent
{
private:
	int map_height, map_width;
	std::vector<Vector2D> DIRS = { DIR_N, DIR_NE, DIR_E, DIR_SE, DIR_S, DIR_SW, DIR_W, DIR_NW };
	std::unique_ptr<MonsterFactory> monsterFactory;
	std::unique_ptr<ItemFactory> itemFactory;
	std::vector<int> dijkstraCosts;

	Vector2D get_map_size() const noexcept
	{
		Vector2D size{ 0, 0 };
		size.x = map_width;
		size.y = map_height;
		return size;
	}
	bool in_bounds(Vector2D pos) const noexcept;

	void init_tiles(GameContext& ctx);
	void place_stairs(GameContext& ctx);
	bool is_stairs(Vector2D pos, GameContext& ctx) const;
	void spawn_water(const DungeonRoom& room, GameContext& ctx);
	bool would_water_block_entrance(Vector2D waterPos, GameContext& ctx) const;
	void spawn_items(const DungeonRoom& room, GameContext& ctx);
	void spawn_barrels(const DungeonRoom& room, GameContext& ctx);
	void spawn_player(const DungeonRoom& room, GameContext& ctx);
	void generate_rooms(bool withActors, GameContext& ctx);
	bool is_floor(Vector2D pos) const noexcept { return get_tile_type(pos) == TileType::FLOOR; }
	bool is_water(Vector2D pos) const noexcept;
	void set_explored(Vector2D pos); // set the tile as explored
	void post_process_doors();

public:
	Map(int map_width, int map_height);

	void load(const json& j) override;
	void save(json& j) override;

	void init(bool withActors, GameContext& ctx);
	bool is_in_fov(Vector2D pos) const noexcept;
	TileType get_tile_type(Vector2D pos) const noexcept;
	void tile_action(Creature& owner, TileType tileType, GameContext& ctx);
	bool is_collision(Creature& owner, TileType tileType, Vector2D pos, GameContext& ctx);
	bool is_explored(Vector2D pos) const noexcept; // indicates whether this tile has already been seen by the player
	bool can_walk(Vector2D pos, const GameContext& ctx) const noexcept;
	void add_monster(Vector2D pos, GameContext& ctx) const;
	void compute_fov(GameContext& ctx);
	void update();
	void render(const GameContext& ctx) const;
	void add_item(Vector2D pos, GameContext& ctx);
	Creature* get_actor(Vector2D pos, const GameContext& ctx) const noexcept; // getActor returns the actor at the given coordinates or NULL if there's none
	std::vector<std::vector<Tile>> get_map() const noexcept;
	void reveal(); // reveal the map
	void regenerate(GameContext& ctx); // regenerate the map
	void spawn_all_enhanced_items_debug(Vector2D position, GameContext& ctx); // debug: spawn all enhanced items
	std::vector<Vector2D> neighbors(Vector2D id, const GameContext& ctx, Vector2D target);
	double cost(Vector2D fromNode, Vector2D toNode, const GameContext& ctx);
	int get_width() const noexcept { return map_width; }
	int get_height() const noexcept { return map_height; }
	long get_seed() const noexcept { return seed; }
	bool is_in_bounds(Vector2D pos) const noexcept { return pos.x >= 0 && pos.x < map_width && pos.y >= 0 && pos.y < map_height; }
	static std::vector<Vector2D> bresenham_line(Vector2D from, Vector2D to);
	size_t get_index(Vector2D pos) const
	{
		if (in_bounds(pos))
		{
			return pos.y * map_width + pos.x;
		}
		else
		{
			throw std::out_of_range{ "Map::get_index() out of bounds" };
		}
	} // Note: Cannot be noexcept due to exception
	double get_cost(Vector2D pos) const noexcept;
	bool has_los(Vector2D from, Vector2D to) const noexcept;
	bool open_door(Vector2D pos, GameContext& ctx);
	bool close_door(Vector2D pos, GameContext& ctx);
	bool unlock_door(Vector2D pos, GameContext& ctx);
	bool is_door_locked(Vector2D pos) const noexcept;
	void open_all_room_doors(Vector2D doorPos, GameContext& ctx);
	void place_amulet(GameContext& ctx);
	std::vector<MonsterPercentage> get_monster_distribution(int dungeonLevel);
	std::vector<ItemPercentage> get_item_distribution(int dungeonLevel);
	void create_treasure_room(const DungeonRoom& room, int quality, GameContext& ctx);
	bool maybe_create_treasure_room(int dungeonLevel, GameContext& ctx);
	Decoration* find_decoration_at(Vector2D pos, const GameContext& ctx) const noexcept;
	bool is_door(Vector2D pos) const noexcept;
	bool is_open_door(Vector2D pos) const noexcept;
	bool is_wall(Vector2D pos) const noexcept;
	int get_dijkstra_cost(Vector2D pos) const noexcept;
	void rebuild_dijkstra_map(const std::vector<Vector2D>& goals, const GameContext& ctx);
	void set_tile(Vector2D pos, TileType newType, double cost);
	void place_from_graph(
		const std::vector<DungeonRoom>& rooms,
		bool withActors,
		GameContext& ctx);

	// Counts doors on this room's wall border that have a room-interior
	// cardinal neighbour. Used to select single-entrance treasure rooms.
	int count_room_entrances(const DungeonRoom& room) const;

	std::vector<Tile> tiles;

protected:
	std::unique_ptr<FovMap> fovMap;
	RandomDice mapRng_;
	long seed;
	friend class DungeonGenerator;
	void dig(Vector2D begin, Vector2D end);
	void dig_corridor(Vector2D begin, Vector2D end);
	void set_door(Vector2D thisTile, int tileX, int tileY, bool locked);
	void setup_treasure_room_guard(const DungeonRoom& room, GameContext& ctx);
	void create_room(const DungeonRoom& room, bool first, bool withActors, GameContext& ctx);
	void spawn_traps(const DungeonRoom& room, GameContext& ctx);
};
