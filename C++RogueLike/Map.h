// file: Map.h
#ifndef MAP_H
#define MAP_H

#include "Goblin.h"
#include "Persistent.h"

class Actor;

//==Tile==
// A tile of the map
// checks if the player has seen this tile
// it is used for field of view algorithm (a 2D array) (see Map.h) (see Map.cpp) (see Map::computeFov()) 
class Tile
{
public:
	bool explored = false;

	//Tile() : explored(false) {} // check if the player has seen this tile set to 'false' by default

	/*bool tile() { return explored = false; }*/
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
	void set_player_pos(int x, int y) { playerPosX = x; playerPosY = y; }
	int get_player_pos_x() const { return playerPosX; }
	int get_player_pos_y() const { return playerPosY; }

	//this is the map dimensions
	int map_height, map_width;

	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;

	//this is the map array
	void bsp(int map_width, int map_height, TCODRandom* rng, bool withActors);
	//the constructor
	Map(int map_height, int map_width);
	//the destructor
	~Map();
	
	//check if a tile is walkable
	bool is_wall(int isWall_pos_y, int isWall_pos_x) const;
	//check if a tile is in the FOV
	bool is_in_fov(int fov_x, int fov_y) const;
	//indicates whether this tile has already been seen by the player
	bool is_explored(int exp_x, int exp_y) const;
	bool can_walk(int canw_x, int canw_y) const;
	void add_monster(int mon_x, int mon_y);
	// compute the field of view using `TCODMap::computeFov()`
	void compute_fov();
	void render() const;
	void add_item(int x, int y);
	int random_number(int min, int max);

	// getActor returns the actor at the given coordinates or NULL if there's none
	std::shared_ptr<Actor> get_actor(int x, int y) const;

	void init(bool withActors);

protected:
	// create a pointer for the map array
	Tile* tiles;
	//create a reference to a TCODMap object named map
	TCODMap* tcodMap;

	long seed;
	TCODRandom* rng;

	//make a friend class for the BspListener
	friend class BspListener;
	//make a dig function for the map
	void dig(int x1, int y1, int x2, int y2);
	//a function for the room generation
	void create_room(bool first, int x1, int y1, int x2, int y2, bool withActors);
};

#endif // !MAP_H
// end of file: Map.h
