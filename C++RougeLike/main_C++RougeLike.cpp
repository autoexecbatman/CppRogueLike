// Debug needs to be set to x86
#include <iostream>
#include <time.h>
#include "main_C++RougeLike.h"


//====
Map::Map(int height, int width) :  height(height) , width(width)
{
    tiles = new Tile[height * width];

    Bsp bsp(0, 0, height, width);

    bsp.mysplitRecursive(NULL, 8, ROOM_MAX_SIZE, ROOM_MAX_SIZE, 1.5f, 1.5f);

    BspListener listener(*this);
    bsp.traverseInvertedLevelOrder(&listener, NULL);

    setWall(22, 30);
    setWall(22, 50);
}

Map::~Map() 
{
    delete[] tiles;
}

bool Map::isWall(int y, int x) const 
{
    return !tiles[y + x * width].canWalk;
}

void Map::setWall(int y, int x) 
{
    tiles[y + x * width].canWalk = false;
}

void Map::render() const
{
    //green
    static const int darkWall = 1;
    //blue
    static const int darkGround = 2;

    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            //TCODConsole::root->setCharBackground(x, y,
            //    isWall(x, y) ? darkWall : darkGround);
            mvchgat(y,x,-1, A_NORMAL,isWall(y,x) ? darkWall : darkGround, NULL);
        }
    }
}

void Map::dig(int y1, int x1, int y2, int x2)
{
    if (x2 < x1)
    {
        int tmp = x2;
        x2 = x1;
        x1 = tmp;
    }

    if (y2 < y1)
    {
        int tmp = y2;
        y2 = y1;
        y1 = tmp;
    }

    for (int tilex = x1; tilex <= x2; tilex++)
    {
        for (int tiley = y1; tiley <= y2; tiley++)
        {
            tiles[tilex + tiley * width].canWalk = true;
        }
    }
    
}

void Map::createRoom(bool first, int y1, int x1, int y2, int x2)
{
    /*Engine engine;*/

    dig(y1, x1, y2, x2);

    if (first)
    {
        //put the player in the first room
        //engine.player->y = (y1 + y2) / 2;
        //engine.player->x = (x1 + x2) / 2;
    }
    else
    {
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

        
        if (int rng = rand() % 4 == 0)
        {
            engine.actors.push_back(
                new Actor(
                (x1 + x2) / 2,
                (y1 + y2) / 2,
                '@',
                COLOR_YELLOW
                )
            );
        }
    }
}

//====
Engine::Engine()
{
    //initscr() should be the first curses routine called.
    //It will initialize all curses data structures, and arrange that the first call to refresh() will clear the screen.
    //In case of error, initscr() will write a message to standard error and end the program.
    initscr();
    start_color();
    player = new Actor(25, 40, '@',4);
    actors.push_back(player);
    /*actors.push_back(new Actor(13, 60,  '@', 4));*/
    map = new Map(45,80);
}

Engine::~Engine() 
{
    actors.clear();
    actors.~vector();
    delete map;
}

void Engine::update()
{
    Engine* engine = 0;
    
    /*TCOD_key_t key;*/
    /*TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS, &key, NULL);*/
    
    int key = getch();

    switch (key)
    {
    //case TCODK_UP:
    //    if (!map->isWall(player->x, player->y - 1)) {
    //        player->y--;
    //    }
    //    break;
    case UP:
        if (!map->isWall(player->y - 1, player->x))
        {
            player->y--;
        }
        break;

    //case TCODK_DOWN:
    //    if (!map->isWall(player->x, player->y + 1)) {
    //        player->y++;
    //    }
    //    break;

    case DOWN:
        if (!map->isWall(player->y + 1, player->x))
        {
            player->y++;
        }
        break;

    //case TCODK_LEFT:
    //    if (!map->isWall(player->x - 1, player->y)) {
    //        player->x--;
    //    }
    //    break;

    case LEFT:
        if (!map->isWall(player->y , player->x - 1))
        {
            player->x--;
        }
        break;

    //case TCODK_RIGHT:
    //    if (!map->isWall(player->x + 1, player->y)) {
    //        player->x++;
    //    }
    //    break;

    case RIGHT:
        if (!map->isWall(player->y,player->x + 1))
        {
            player->x++;
        }
        break;

    case QUIT:
        engine->quit = true;
        break;

    default:
        break;
    }
}

void Engine::render() 
{
    clear();
    // draw the map
    map->render();

    //// draw the actors
    //for (Actor* iterator = actors.begin(); iterator != actors.end(); iterator++) 
    //{
    //    (*iterator)->render();
    //}

    // draw the actors
    for (auto actor : actors)
    {
        actor->render();
    }
}

//====
Actor::Actor(int y, int x, int ch, int col) : y(y), x(x), ch(ch), col(col)
{
}

void Actor::render() const
{
    attron(COLOR_PAIR(col));
    mvaddch(y, x, ch);
    attroff(COLOR_PAIR(col));
}
//====

int main()
{

    

//====
#define GRASS_PAIR     1
#define EMPTY_PAIR     1
#define WATER_PAIR     2
#define MOUNTAIN_PAIR  3
#define NPC_PAIR       4
#define PLAYER_PAIR    5

        init_pair(1, COLOR_YELLOW, COLOR_GREEN);
        init_pair(2, COLOR_CYAN, COLOR_BLUE);
        init_pair(3, COLOR_BLACK, COLOR_WHITE);
        init_pair(4, COLOR_RED, COLOR_MAGENTA);
        init_pair(5, COLOR_RED, COLOR_YELLOW );
        
//====
        /*bool quit = false;*/
    while (/*true*/!engine.quit == true)
    {
        engine.update();
        engine.render();
        refresh();
    }
    return 0;
}