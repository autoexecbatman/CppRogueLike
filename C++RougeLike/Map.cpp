#include <curses.h>

#include "libtcod.hpp"

#include "Map.h"
#include "Actor.h"
#include "Engine.h"

static const int ROOM_MAX_SIZE = 12;
static const int ROOM_MIN_SIZE = 6;

//create a binary space partition listener class (BSP)
class BspListener : public ITCODBspCallback
{
private:
    Map& map; // a map to dig
    int roomNum; // room number
    int lastx = 0, lasty = 0; // center of the last room
public:
    BspListener(Map& map) : map(map), roomNum(0) {}

    bool visitNode(TCODBsp* node, void* userData)
    {
        if (node->isLeaf())
        {
            // variables
            int room_pos_x = 0, room_pos_y = 0, room_width = 0, room_height = 0;
            // dig a room
            TCODRandom* rng = TCODRandom::getInstance();
            room_width = rng->getInt(ROOM_MIN_SIZE, node->w - 2);//random int from min size to width - 2
            room_height = rng->getInt(ROOM_MIN_SIZE, node->h - 2);//random int from min size to height - 2
            room_pos_x = rng->getInt(node->x + 1, node->x + node->w - room_width - 1);//from node x + 1 to node x + node width - width - 1
            room_pos_y = rng->getInt(node->y + 1, node->y + node->h - room_height - 1);//from node y + 1 to node x + node height - width - 1

            map.createRoom//create rooms func
            (
                roomNum == 0,
                room_pos_x,
                room_pos_y,
                room_pos_x + room_width - 1 - 1,
                room_pos_y + room_height - 1 - 1
            );
            
            if (roomNum != 0)
            {
                // dig a corridor from last room

                map.dig(
                    room_pos_x + room_width / 2,
                    lasty,
                    room_pos_x + room_width / 2,
                    room_pos_y + room_height / 2
                );
                map.dig(
                    lastx,
                    lasty,
                    room_pos_x + room_width / 2,
                    lasty
                );
                /*map.dig(1,10,117,10);*/
            }
            //set variables
            lastx = room_pos_x + room_width / 2;
            lasty = room_pos_y + room_height / 2;
			
            //iterate to next room
            roomNum++;
        }
        return true;
    }
};


void Map::bsp(int map_width, int map_height)
{
    TCODBsp* myBSP = new TCODBsp(0, 0, map_width, map_height);
    myBSP->splitRecursive(NULL, 4, ROOM_MAX_SIZE, ROOM_MAX_SIZE, 1.5f, 1.5f);
    BspListener* mylistener = new BspListener(*this);
    myBSP->traverseInvertedLevelOrder(mylistener, NULL);
}

//====
//In Map.cpp, we allocate the TCODMap object in the constructor
Map::Map(int map_height, int map_width) : map_height(map_height), map_width(map_width)
{
    tiles = new Tile[map_height * map_width];
    map = new TCODMap(map_width, map_height);
    bsp(map_width, map_height);
}

Map::~Map()
{
    delete[] tiles;
    delete map;
}

bool Map::isWall(int isWall_pos_y, int isWall_pos_x) const//checks if it is a wall?
{
    //return !tiles[isWall_pos_x + isWall_pos_y * map_width].canWalk;

    return !map->isWalkable(
        isWall_pos_x,
        isWall_pos_y
    );

}

bool Map::isExplored(int exp_x, int exp_y) const
{
    return tiles[exp_x + exp_y * map_width].explored;
}

bool Map::isInFov(int fov_x, int fov_y) const
{
    if (map->isInFov(fov_x,fov_y))
    {
        tiles[fov_x + fov_y * map_width].explored = true;
        return true;
    }
    return false;
}

void Map::computeFov()
{
    map->computeFov(engine.player->x, engine.player->y, engine.fovRadius);
}

void Map::render() const
{
    static const int darkWall = 1;//green
    static const int darkGround = 2;//blue
    static const int lightWall = 3;//
    static const int lightGround = 4;//

    for (int iter_y = 0; iter_y < map_height; iter_y++)
    {
        for (int iter_x = 0; iter_x < map_width; iter_x++)
        {
            if (isInFov(iter_x,iter_y))
            {
                mvchgat(iter_y, iter_x, 1, A_NORMAL, isWall(iter_y, iter_x) ? lightWall : lightGround, NULL);
            }
            else if (isExplored(iter_x,iter_y))
            {
                mvchgat(iter_y, iter_x, 1, A_NORMAL, isWall(iter_y, iter_x) ? darkWall : darkGround, NULL);
            }
            /*mvchgat(iter_y, iter_x, 1, A_NORMAL, isWall(iter_y, iter_x) ? darkWall : darkGround, NULL);*/
        }
    }
}
//a function for digging the tiles
void Map::dig(int x1, int y1, int x2, int y2)
{
    //swap x
    if (x2 < x1)
    {
        //printw("x1:%u|", x1);
        //printw("y1:%u", x1);
        //printw("num:%u\n", iter);
        int tmp = x2;
        x2 = x1;
        x1 = tmp;
    }
    //swap y
    if (y2 < y1)
    {
        int tmp = y2;
        y2 = y1;
        y1 = tmp;
    }
    //DEBUG
    //printw("x1:%u~", x1);
    //printw("y1:%u|", x1);

    //We're using a fovRadius field on the engine class, 
    //this way we'll be able to dynamically change the radius.
    //the isInFov function will be used to check if the tile is in the fov.
	//The dig function must also be updated to use the TCODMap object
    for (int tiley = y1; tiley <= y2 +1; tiley++)
    {
        for (int tilex = x1; tilex <= x2 +1; tilex++)
        {
            //tiles[tilex + tiley * map_width].canWalk = true;

            //set the map properties
			map->setProperties(
				tilex,
				tiley,
				true,
				true
			);
			
            /*map->setProperties(tilex, tiley, true, true);*/
			
        }
    }
}


//the implementation of the Map class
void Map::createRoom(bool first, int x1, int y1, int x2, int y2)
{
	//dig the corridors
    dig(x1, y1, x2, y2);

    //if this is the first room, we need to place the player in it
    if (first)
    {
        engine.player->y = y1 + 1;
        engine.player->x = x1 + 1;
    }
	//if this is not the first room, we make a random number of monsters and place them in the room
    else
    {
        if (int rng = rand() % 4 == 0)
        {
            engine.actors.push_back(
                new Actor(
                    (y1 + y2) / 2,
                    (x1 + x2) / 2,
                    '@',
                    4
                )
            );
        }
    }
    
}
//====
