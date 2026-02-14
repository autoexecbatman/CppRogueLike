#pragma once

#include <unordered_set>
#include <memory>
#include <vector>

#include <libtcod.h>

#include "../Persistent/Persistent.h"
#include "../Actor/Actor.h"
#include "../Factories/MonsterFactory.h"
#include "../Factories/ItemFactory.h"

// Forward declaration
struct GameContext;

inline constexpr int DEFAULT_MAP_WIDTH = 120;
inline constexpr int DEFAULT_MAP_HEIGHT = 80;

inline int get_map_width() { return DEFAULT_MAP_WIDTH; }
inline int get_map_height() { return DEFAULT_MAP_HEIGHT; }

inline constexpr int FOV_RADIUS = 4;

inline constexpr auto ROOM_MAX_SIZE = 12;
inline constexpr auto ROOM_HORIZONTAL_MAX_SIZE = 20;
inline constexpr auto ROOM_VERTICAL_MAX_SIZE = 10;
inline constexpr auto ROOM_MIN_SIZE = 6;
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

struct Tile
{
	Vector2D position;
	TileType type;
	bool explored;
	double cost;

	// overload the greater than operator
	bool operator>(const Tile& other) const { return cost > other.cost; }
	Tile(Vector2D pos, TileType type, double cost) : position(pos), type(type), cost(cost), explored(false) {}
};

//==Map==
//a class for the game map
//the map is a 2d array of tiles
class Map : public Persistent
{
private:
	int map_height, map_width;
	std::vector<Vector2D> DIRS =
	{
		Vector2D{-1, 0}, // y, x North
		Vector2D{-1, 1}, // y, x North-West
		Vector2D{0, 1}, // y, x West
		Vector2D{1, 1}, // y, x South-West
		Vector2D{1, 0}, // y, x South
		Vector2D{1, -1}, // y, x South-East
		Vector2D{0, -1}, // y, x East
		Vector2D{-1, -1} // y, x North-East
	};
	std::unique_ptr<MonsterFactory> monsterFactory;
	std::unique_ptr<ItemFactory> itemFactory;

	Vector2D get_map_size() const noexcept { Vector2D size{ 0,0 }; size.x = map_width; size.y = map_height; return size; }
	bool in_bounds(Vector2D pos) const noexcept;

	void init_tiles(GameContext& ctx);
	void place_stairs(GameContext& ctx);
	bool is_stairs(Vector2D pos, GameContext& ctx) const;
	void spawn_water(Vector2D begin, Vector2D end, GameContext& ctx);
	bool would_water_block_entrance(Vector2D waterPos, GameContext& ctx) const;
	void spawn_items(Vector2D begin, Vector2D end, GameContext& ctx);
	void spawn_player(Vector2D begin, Vector2D end, GameContext& ctx);
	void bsp(int map_width, int map_height, TCODRandom& rng_unique, bool withActors, GameContext& ctx);
	bool is_floor(Vector2D pos) const noexcept { return get_tile_type(pos) == TileType::FLOOR;}
	bool is_water(Vector2D pos) const noexcept;
	void set_explored(Vector2D pos); //set the tile as explored
	void post_process_doors();
public:

	Map(int map_height, int map_width);

	void load(const json& j) override;
	void save(json& j) override;

	void init(bool withActors, GameContext& ctx);
	bool is_in_fov(Vector2D pos) const noexcept;
	TileType get_tile_type(Vector2D pos) const noexcept;
	void tile_action(Creature& owner, TileType tileType, GameContext& ctx);
	bool is_collision(Creature& owner, TileType tileType, Vector2D pos, GameContext& ctx);
	bool is_explored(Vector2D pos) const noexcept; //indicates whether this tile has already been seen by the player
	bool can_walk(Vector2D pos, GameContext& ctx) const noexcept;
	void add_monster(Vector2D pos, GameContext& ctx) const;
	void compute_fov(GameContext& ctx); // compute the field of view using `TCODMap::computeFov()`
	void update();
	void render(const GameContext& ctx) const;
	void add_item(Vector2D pos, GameContext& ctx);
	Creature* get_actor(Vector2D pos, GameContext& ctx) const noexcept; // getActor returns the actor at the given coordinates or NULL if there's none
	std::vector<std::vector<Tile>> get_map() const noexcept;
	void reveal(); // reveal the map
	void regenerate(GameContext& ctx); // regenerate the map
	void spawn_all_enhanced_items_debug(Vector2D position, GameContext& ctx); // debug: spawn all enhanced items
	std::vector<Vector2D> neighbors(Vector2D id, GameContext& ctx, Vector2D target = Vector2D{-1, -1});
	double cost(Vector2D from_node, Vector2D to_node, GameContext& ctx);
	int get_width() const noexcept { return map_width; }
	int get_height() const noexcept { return map_height; }
	bool is_in_bounds(Vector2D pos) const noexcept { return pos.x >= 0 && pos.x < map_width && pos.y >= 0 && pos.y < map_height; }
	static std::vector<Vector2D> bresenham_line(Vector2D from, Vector2D to);
	size_t get_index(Vector2D pos) const { if (in_bounds(pos)) { return pos.y * map_width + pos.x; } else { throw std::out_of_range{ "Map::get_index() out of bounds" }; } } // Note: Cannot be noexcept due to exception
	double get_cost(Vector2D pos, GameContext& ctx) const noexcept;
	bool has_los(Vector2D from, Vector2D to) const noexcept;
	bool open_door(Vector2D pos, GameContext& ctx);
	bool close_door(Vector2D pos, GameContext& ctx);
	void place_amulet(GameContext& ctx);
	void display_spawn_rates(GameContext& ctx) const;
	void create_treasure_room(Vector2D begin, Vector2D end, int quality, GameContext& ctx);
	bool maybe_create_treasure_room(int dungeonLevel, GameContext& ctx);
	void display_item_distribution(GameContext& ctx) const;
	bool is_door(Vector2D pos) const noexcept;
	bool is_open_door(Vector2D pos) const noexcept;
	bool is_wall(Vector2D pos) const noexcept;
	void set_tile(Vector2D pos, TileType newType, double cost);

	std::unique_ptr<TCODPath> tcodPath;
	std::vector<Tile> tiles;
protected:
	std::unique_ptr<TCODMap> tcodMap;
	std::unique_ptr<TCODRandom> rng_unique;
	long seed;
	friend class BspListener;
	friend class PathListener;
	void dig(Vector2D begin, Vector2D end);
	void dig_corridor(Vector2D begin, Vector2D end);
	void set_door(Vector2D thisTile, int tileX, int tileY);
	void create_room(bool first, int x1, int y1, int x2, int y2, bool withActors, GameContext& ctx);
};
