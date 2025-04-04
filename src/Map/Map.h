#pragma once

#include <unordered_set>

#include "../Persistent/Persistent.h"
#include "../Actor/Actor.h"
#include "../MonsterFactory.h"

inline constexpr int MAP_HEIGHT = 30 - 8;
inline constexpr int MAP_WIDTH = 119;

inline constexpr int FOV_RADIUS = 10;

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

	Vector2D get_map_size() const noexcept { Vector2D size{ 0,0 }; size.x = map_width; size.y = map_height; return size; }
	bool in_bounds(Vector2D pos) const noexcept { return pos <= Vector2D{ 0,0 } || pos <= get_map_size(); }
	
	void init_tiles();
	void place_stairs() const;
	bool is_stairs(Vector2D pos) const;
	void spawn_water(Vector2D begin, Vector2D end);
	void spawn_items(Vector2D begin, Vector2D end);
	void spawn_player(Vector2D begin, Vector2D end);
	void bsp(int map_width, int map_height, TCODRandom& rng_unique, bool withActors);
	bool is_floor(Vector2D pos) const { return get_tile_type(pos) == TileType::FLOOR;}
	bool is_water(Vector2D pos) const;
	void set_explored(Vector2D pos); //set the tile as explored
public:

	Map(int map_height, int map_width);

	void load(const json& j) override;
	void save(json& j) override;

	void init(bool withActors);
	bool is_in_fov(Vector2D pos) const;
	TileType get_tile_type(Vector2D pos) const;
	void tile_action(Creature& owner, TileType tileType);
	bool is_collision(Creature& owner, TileType tileType, Vector2D pos);
	bool is_explored(Vector2D pos) const; //indicates whether this tile has already been seen by the player
	bool can_walk(Vector2D pos) const;
	void add_monster(Vector2D pos);
	void compute_fov(); // compute the field of view using `TCODMap::computeFov()`
	void update();
	void render() const;
	void add_weapons(Vector2D pos);
	void add_item(Vector2D pos);
	Creature* get_actor(Vector2D pos) noexcept; // getActor returns the actor at the given coordinates or NULL if there's none
	std::vector<std::vector<Tile>> get_map() const;
	void reveal(); // reveal the map
	void regenerate(); // regenerate the map
	std::vector<Vector2D> neighbors(Vector2D id) const;
	double cost(Vector2D from_node, Vector2D to_node);
	int get_width() const noexcept { return map_width; }
	int get_height() const noexcept { return map_height; }
	size_t get_index(Vector2D pos) const { if (in_bounds(pos)) { return pos.y * map_width + pos.x; } else { throw std::out_of_range{ "Map::get_index() out of bounds" }; } }
	double get_cost(Vector2D pos) const noexcept { return tiles.at(get_index(pos)).cost; }
	bool has_los(Vector2D from, Vector2D to) const;
	bool open_door(Vector2D pos);
	bool close_door(Vector2D pos);
	void place_amulet() const;
	void display_spawn_rates() const;
	bool is_door(Vector2D pos) const;
	bool is_wall(Vector2D pos) const;
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
	void create_room(bool first, int x1, int y1, int x2, int y2, bool withActors);
};
