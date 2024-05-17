// file: Map.h
#ifndef MAP_H
#define MAP_H

#pragma once

#include "../Persistent/Persistent.h"
#include "../ActorTypes/Monsters.h"

class Actor;

inline constexpr int MAP_HEIGHT = 30 - 8;
inline constexpr int MAP_WIDTH = 119;

inline constexpr int FOV_RADIUS = 10;

inline constexpr auto ROOM_MAX_SIZE = 12;
inline constexpr auto ROOM_MIN_SIZE = 6;
inline constexpr auto MAX_ROOM_MONSTERS = 1; // TODO: check if this is used at all 
inline constexpr int MAX_ROOM_ITEMS = 4;

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
	// Add more as needed...
};

class Tile
{
public:
	TileType type{};
	bool explored{ false };
};

//==Map==
//a class for the game map
//the map is a 2d array of tiles
class Map : public Persistent
{
private:
	int playerPosX{ 0 };
	int playerPosY{ 0 };
public:
	int map_height{}, map_width{};

	Map(int map_height, int map_width);

	template <typename MonsterType>
	std::unique_ptr<MonsterType> create_monster(Vector2D position)
	{
		auto monster = std::make_unique<MonsterType>(position);
		return monster;
	}
	void set_player_pos(int x, int y) noexcept { playerPosX = x; playerPosY = y; }
	int get_player_pos_x() const noexcept { return playerPosX; }
	int get_player_pos_y() const noexcept { return playerPosY; }

	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;

	void bsp(int map_width, int map_height, TCODRandom& rng_unique, bool withActors);

	void init(bool withActors);
	bool is_wall(int isWall_pos_y, int isWall_pos_x) const;
	bool is_in_fov(Vector2D position) const;
	bool is_water(int isWater_pos_y, int isWater_pos_x) const;
	bool is_explored(int exp_x, int exp_y) const noexcept; //indicates whether this tile has already been seen by the player
	bool can_walk(int canw_x, int canw_y) const;
	void add_monster(int mon_x, int mon_y);
	void compute_fov(); // compute the field of view using `TCODMap::computeFov()`
	void render() const;
	void add_item(int x, int y);
	Actor* get_actor(int x, int y) noexcept; // getActor returns the actor at the given coordinates or NULL if there's none

protected:
	std::unique_ptr<Tile[]> tiles{};
	std::unique_ptr<TCODMap> tcodMap{};
	std::unique_ptr<TCODRandom> rng_unique{};
	long seed{};

	friend class BspListener;
	void dig(int x1, int y1, int x2, int y2);
	void set_tile(int x, int y, TileType newType) noexcept;
	void create_room(bool first, int x1, int y1, int x2, int y2, bool withActors);
};

#endif // !MAP_H
// end of file: Map.h
