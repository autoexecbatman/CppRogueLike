#include <curses.h>

#include "libtcod.hpp"

#include "Map.h"
#include "Actor.h"
#include "Engine.h"

static const int ROOM_MAX_SIZE = 12;
static const int ROOM_MIN_SIZE = 6;


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
            //printw("isLeaf\n");
            //printw("%u",node->isLeaf());
            printw("|node->x%u", node->x);
            printw("|node->w%u", node->w);
            printw("|node-y%u", node->y);
            printw("|node-h%u", node->h);
            printw("|room%u\n", roomNum);
            int room_pos_x = 0, room_pos_y = 0, room_width = 0, room_height = 0;
            // dig a room
            TCODRandom* rng = TCODRandom::getInstance();
            room_width = rng->getInt(ROOM_MIN_SIZE, node->w - 2);//random int from min size to width - 2
            room_height = rng->getInt(ROOM_MIN_SIZE, node->h - 2);//random int from min size to height - 2
            room_pos_x = rng->getInt(node->x + 1, node->x + node->w - room_width - 1);//from node x + 1 to node x + node width - width - 1
            room_pos_y = rng->getInt(node->y + 1, node->y + node->h - room_height - 1);//from node y + 1 to node x + node height - width - 1
            
            ////dig a static room
            //int rw = 10;
            //int rh = 5;
            //int rx = 5;
            //int ry = 5;

            //map.createRoom(roomNum == 0, rx, ry, rx + rw - 1, ry + rh - 1);
            //map.createRoom(roomNum == 1, 100, 5, 89, 12);
            ///*map.createRoom(roomNum == 1, 30, 10, 4, 14);*/

            //if (roomNum != 0)
            //{
            //    map.dig(lasty,rx+rw/2,ry+rh/2,rx+rw/2);
            //    map.dig(lasty, lastx, lasty, rx + rw / 2);
            //}


            //lastx = rx + rw / 2;
            //lasty = ry + rh / 2;
            /*roomNum++;*/
            ////DEBUG rng
            //printw("w = %u||", w);
            //printw("h = %u||", h);
            //printw("x = %u||", x);
            //printw("y = %u||", y);
            //printw("roomnum = %u\n", roomNum);

            map.createRoom//createroom func
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

                //map.dig(lasty, room_pos_x + room_width / 2, room_pos_y + room_height / 2, room_pos_x + room_width / 2);
                //map.dig(lasty, lastx, lasty, room_pos_x + room_width / 2);
                map.dig(1,10,117,10);
            }

            lastx = room_pos_x + room_width / 2;
            lasty = room_pos_y + room_height / 2;
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
Map::Map(int map_height, int map_width) : map_height(map_height), map_width(map_width)
{
    tiles = new Tile[map_height * map_width];
    bsp(map_width, map_height);
    /*printw("map_height:%u", map_height);*/
    //int room_vertical_size = 40;
    //int room_horizontal_size = 20;
    //for (int i = 0; i < room_vertical_size; ++i)
    //{
    //    for (int j = 0; j < room_horizontal_size; ++j)
    //    {
    //        setWall(j, i);
    //    }        
    //}
    /*setWall(22, 40);*/
    /*TCODBsp* myBSP = new TCODBsp(0, 0, map_width, map_height);*/

}

Map::~Map()
{
    delete[] tiles;
}

bool Map::isWall(int isWall_pos_y, int isWall_pos_x) const//checks if it is a wall?
{
    return !tiles[isWall_pos_x + isWall_pos_y * map_width].canWalk;
}

void Map::setWall(int setWall_pos_y, int setWall_pos_x)
{
    tiles[setWall_pos_x + setWall_pos_y * map_width].canWalk = false;
}

void Map::render() const
{
    static const int darkWall = 1;//green
    static const int darkGround = 2;//blue

    for (int iter_y = 0; iter_y < map_height; iter_y++)
    {
        for (int iter_x = 0; iter_x < map_width; iter_x++)
        {
            mvchgat(iter_y, iter_x, 1, A_NORMAL, isWall(iter_y, iter_x) ? darkWall : darkGround, NULL);
        }
    }
}

void Map::dig(int x1, int y1, int x2, int y2)
{

    //int tiley = y1+20;
    //int tilex = x1+20;


    //tiles[tilex + tiley * width].canWalk = true;
    //printw("x1:%u|", x1);
    //printw("y1:%u|\n", x1);

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

    //printw("x1:%u~", x1);
    //printw("y1:%u|", x1);


    for (int tiley = y1; tiley <= y2 +1; tiley++)
    {
        for (int tilex = x1; tilex <= x2 +1; tilex++)
        {
            tiles[tilex + tiley * map_width].canWalk = true;
        }
    }
}
//Engine* engine = new Engine;
void Map::createRoom(bool first, int x1, int y1, int x2, int y2)
{
   
    dig(x1, y1, x2, y2);

    if (first)
    {
        engine.player->y = y1 + 1;
        engine.player->x = x1 + 1;
    }
    else
    {
        if (int rng = rand() % 4 == 0)
        {
            engine.actors.push_back(
                new Actor(
                    (x1 + x2) / 2,
                    (y1 + y2) / 2,
                    '@',
                    4
                )
            );
        }
        //TCODRandom random;
        //std::default_random_engine* generator;
        //std::uniform_int_distribution<int> distribution(1, 6);
        //auto dice = std::bind(distribution, generator);
        //auto rng = dice;

        /*sp::StaticRandomInitializer* rng;*/
        //sp::StaticRandomInitializer::StaticRandomInitializer()* rng = 0;
        /*sp::StaticRandomInitializer::StaticRandomInitializer();*/
        //auto* rng = sp::irandom;



        /*srand(time(NULL));*/

        //int* ptr_rng;
        //int* ptr_rng = &rngrand;


        
    }
    
}
//====