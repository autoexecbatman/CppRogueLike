// file: Map.h
#ifndef MAP_H
#define MAP_H

#pragma once

#include "../Persistent/Persistent.h"
#include "../Actor/Actor.h"

class Actor;
class Creature;

inline constexpr int MAP_HEIGHT = 30 - 8;
inline constexpr int MAP_WIDTH = 119;

inline constexpr int FOV_RADIUS = 10;

inline constexpr auto ROOM_MAX_SIZE = 12;
inline constexpr auto ROOM_HORIZONTAL_MAX_SIZE = 20;
inline constexpr auto ROOM_VERTICAL_MAX_SIZE = 10;
inline constexpr auto ROOM_MIN_SIZE = 6;
inline constexpr int MAX_ROOM_ITEMS = 4;
inline constexpr int MAX_MONSTERS = 6;

//==Tile==
// A tile of the map
// checks if the player has seen this tile
// it is used for field of view algorithm (a 2D array) (see Map.h) (see Map.cpp) (see Map::computeFov())

enum class TileType
{
	FLOOR,
	WALL,
	WATER,
	DOOR,
	CORRIDOR,
	// Add more as needed...
};

struct Tile
{
	Vector2D position;
	TileType type;
	bool explored;
	Tile(Vector2D pos, TileType type) : position(pos), type(type), explored(false) {}	
};

//==Map==
//a class for the game map
//the map is a 2d array of tiles
class Map : public Persistent
{
private:
	Vector2D get_map_size() const noexcept { Vector2D size{ 0,0 }; size.x = map_width; size.y = map_height; return size; }
	bool in_bounds(Vector2D pos) const noexcept { return pos <= Vector2D{ 0,0 } || pos <= get_map_size(); }
	size_t get_index(Vector2D pos) const { if (in_bounds(pos)) { return pos.y * map_width + pos.x; } else {throw std::out_of_range { "Map::get_index() out of bounds" };} }
	void init_tiles();
	void place_stairs() const;
	bool is_stairs(Vector2D pos) const;
	void spawn_water(Vector2D begin, Vector2D end);
	void spawn_items(Vector2D begin, Vector2D end);
	void spawn_player(Vector2D begin, Vector2D end);
	void bsp(int map_width, int map_height, TCODRandom& rng_unique, bool withActors);
	bool is_wall(Vector2D pos) const;
	bool is_floor(Vector2D pos) const { return get_tile_type(pos) == TileType::FLOOR;}
	bool is_water(Vector2D pos) const;
	void set_explored(Vector2D pos); //set the tile as explored

	int map_height, map_width;
public:

	Map(int map_height, int map_width);

	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;

	void init(bool withActors);
	bool is_in_fov(Vector2D pos) const;
	TileType get_tile_type(Vector2D pos) const;
	bool tile_action(TileType tileType);
	bool is_explored(Vector2D pos) const; //indicates whether this tile has already been seen by the player
	bool can_walk(Vector2D pos) const;
	void add_monster(Vector2D pos);
	void compute_fov(); // compute the field of view using `TCODMap::computeFov()`
	void update();
	void render() const;
	void add_weapons(Vector2D pos);
	void add_item(Vector2D pos);
	Creature* get_actor(Vector2D pos) noexcept; // getActor returns the actor at the given coordinates or NULL if there's none
	
	void reveal(); // reveal the map
	void regenerate(); // regenerate the map

	std::unique_ptr<TCODPath> tcodPath;
protected:
	std::vector<Tile> tiles;
	std::unique_ptr<TCODMap> tcodMap;
	std::unique_ptr<TCODRandom> rng_unique;
	long seed;
	friend class BspListener;
	friend class PathListener;
	void dig(Vector2D begin, Vector2D end);
	void dig_corridor(Vector2D begin, Vector2D end);
	void set_tile(Vector2D pos, TileType newType);
	void create_room(bool first, int x1, int y1, int x2, int y2, bool withActors);
};

#endif // !MAP_H
// end of file: Map.h
