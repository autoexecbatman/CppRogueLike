#pragma once

struct Tile
{
	bool canWalk; // can we walk through this tile?
	Tile() : canWalk(false) {}
};

class Map
{
public:
	int map_height, map_width;

	void bsp(int map_width, int map_height);

	Map(int map_height, int map_width);
	~Map();
	bool isWall(int isWall_pos_y, int isWall_pos_x) const;
	void render() const;

protected:
	Tile* tiles;

	void setWall(int setWall_pos_y, int setWall_pos_x);
	friend class BspListener;

	void dig(int x1, int y1, int x2, int y2);
	void createRoom(bool first, int x1, int y1, int x2, int y2);
};