#include <curses.h>
#include <iostream>
#include "Actor.h"
#include "Map.h"
#include "Engine.h"

Engine::Engine()
{
    /*TCODConsole::initRoot(80, 50, "libtcod C++ tutorial", false);*/
    std::cout << "constructed" << std::endl;
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
