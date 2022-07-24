#pragma once
//#include "mybsp.h"
#include "libtcod.hpp"


//static const int ROOM_MAX_SIZE = 12;
//static const int ROOM_MIN_SIZE = 6;

enum CONTROLS
{
	UP = 'w', DOWN = 's', LEFT = 'a', RIGHT = 'd', QUIT = 'q'
};

struct Tile
{
	bool canWalk; // can we walk through this tile?
	Tile() : canWalk(true) {}
};

class Map
{
public:
	int height, width;

	Map(int height, int width);
	~Map();
	bool isWall(int y, int x) const;
	void render() const;
protected:
	Tile* tiles;

	void setWall(int y, int x);
	//friend class BspListener;

	//void dig(int y1,int x1, int y2,int x2);
	//void createRoom(bool first, int y1, int x1, int y2, int x2);
};


class Actor
{
public:
	int y, x;//position on map
	int ch;//ascii code
	int col; //TCODColor col;//color

	Actor(int y, int x, int ch, int col/*const TCODColor& col */);
	void render() const;
};

class Engine
{
public:
	std::vector<Actor*> actors;
	Actor* player;
	Map* map;

	Engine();
	~Engine();
	void update();
	void render();
};
/*extern Engine engine;*/