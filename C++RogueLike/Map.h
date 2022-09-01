#pragma once

//====
//check if the player has seen this tile
struct Tile
{
	bool explored = false;

	//Tile() : explored(false) {} // check if the player has seen this tile set to 'false' by default

	/*bool tile() { return explored = false; }*/
};

//a class for the game map
//the map is a 2d array of tiles
class Map
{
public:
	//this is the map dimensions
	int map_height, map_width;

	//this is the map array
	void bsp(int map_width, int map_height);
	//the constructor
	Map(int map_height, int map_width);
	//the destructor
	~Map();
	
	//check if a tile is walkable
	bool isWall(int isWall_pos_y, int isWall_pos_x) const;
	//check if a tile is in the FOV
	bool isInFov(int fov_x, int fov_y) const;
	//indicates whether this tile has already been seen by the player
	bool isExplored(int exp_x, int exp_y) const;
	bool canWalk(int canw_x, int canw_y) const;
	//create a declaration for addMonster function
	void addMonster(int mon_x, int mon_y);
	//compute the field of view
	void computeFov();
	void render() const;
	void addItem(int x, int y);

protected:
	//create a pointer for the map array
	Tile* tiles;
	//create a reference to a TCODMap object named map
	TCODMap* map;
	//make a friend class for the BspListener
	friend class BspListener;
	//make a dig function for the map
	void dig(int x1, int y1, int x2, int y2);
	//a function for the room generation
	void createRoom(bool first, int x1, int y1, int x2, int y2);
};