// Debug needs to be set to x86
#include <curses.h>
#include "main_C++RougeLike.h"

//====
Map::Map(int height, int width) : height(height), width(width)
{
    tiles = new Tile[height * width];
    setWall(22, 30);
    setWall(22, 40);

    ////Bsp bsp(0, 0, height, width);
    ////bsp.mysplitRecursive(NULL, 8, ROOM_MAX_SIZE, ROOM_MAX_SIZE, 1.5f, 1.5f);
    ////BspListener listener(*this);
    ////bsp.traverseInvertedLevelOrder(&listener, NULL);
    //TCODBsp bsp(0, 0, width, height);
    ////TCODBsp* ptrbsp = 0;
    //
    //bsp.splitRecursive(NULL, 8, ROOM_MAX_SIZE, ROOM_MAX_SIZE, 1.5f, 1.5f);
    //BspListener listener(*this);
    //bsp.traverseInvertedLevelOrder(&listener, NULL);

}

Map::~Map()
{
    delete[] tiles;
}

bool Map::isWall(int y, int x) const
{
    return !tiles[x + y * width].canWalk;
}

void Map::setWall(int y, int x)
{
    tiles[x + y * width].canWalk = false;
}

void Map::render() const
{
    static const int darkWall = 1;//green
    static const int darkGround = 2;//blue

    for (int x = 0; x < width; x++)//if x smaller then map width
    {
        for (int y = 0; y < height; y++)//if y smaller then map width
        {
            //TCODConsole::root->setCharBackground(x, y,
            //    isWall(x, y) ? darkWall : darkGround);
            //mvchgat(y,x,-1, A_NORMAL,isWall(y,x) ? darkWall : darkGround, NULL);
            /*mvaddch(y,x,'#');*/
            mvchgat(y, x, 1, A_NORMAL, isWall(y, x) ? darkWall : darkGround, NULL);
        }
    }
}

Engine::Engine()
{
    /*TCODConsole::initRoot(80, 50, "libtcod C++ tutorial", false);*/
    initscr();
    start_color();
    player = new Actor(25, 40, '@', 3);
    actors.push_back(player);
    /*map = new Map(30, 120);*/
    map = new Map(30, 120);
}

Engine::~Engine()
{
    /*actors.clearAndDelete();*/
    actors.clear();
    delete map;
}

void Engine::update()
{
    //DEBUG
    mvprintw(1, 1, "player->y:%u", player->y);
    mvprintw(2, 1, "player->x:%u", player->x);

    //TCOD_key_t key;
    //TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS, &key, NULL);
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
        if (!map->isWall(player->y, player->x - 1))
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
        if (!map->isWall(player->y, player->x + 1))
        {
            player->x++;
        }
        break;

    default:break;
    }
}

void Engine::render()
{
    //TCODConsole::root->clear();
    clear();
    // draw the map
    map->render();

    // draw the actors
    for (auto actor : actors)
    {
        actor->render();
    }
}
//====
Actor::Actor(int y, int x, int ch, int col) : y(y), x(x), ch(ch), col(col)
{}

void Actor::render() const
{
    //TCODConsole::root->setChar(x, y, ch);
    //TCODConsole::root->setCharForeground(x, y, col);
    attron(COLOR_PAIR(col));
    mvaddch(y, x, ch);
    attroff(COLOR_PAIR(col));
}
//====

int main()
{
    Engine engine;
    //====
#define GRASS_PAIR     1
#define EMPTY_PAIR     1
#define WATER_PAIR     2
#define MOUNTAIN_PAIR  3
#define NPC_PAIR       4
#define PLAYER_PAIR    5
//====
    init_pair(1, COLOR_YELLOW, COLOR_GREEN);
    init_pair(2, COLOR_CYAN, COLOR_BLUE);
    init_pair(3, COLOR_BLACK, COLOR_WHITE);
    init_pair(4, COLOR_RED, COLOR_MAGENTA);
    init_pair(5, COLOR_RED, COLOR_YELLOW);
    //====
    while (true)
    {
        engine.update();
        engine.render();
        refresh();//TCODConsole::flush();
    }
    return 0;
}