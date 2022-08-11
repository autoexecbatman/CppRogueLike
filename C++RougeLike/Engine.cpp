#include <curses.h>
#include <iostream>
#include "Actor.h"
#include "Map.h"
#include "Engine.h"

//the constructor for the engine class
Engine::Engine() : fovRadius(10),computeFov(true)
{
	//initialize the screen in NCurses
    initscr();

	//start color NCurses
    start_color();
	
	//make a new Actor for the player
    player = new Actor(25, 40, '@', 3);
    actors.push_back(player);

	//make a new map instance
    map = new Map(30, 120);

}

//the destructor for the engine class
Engine::~Engine()
{
    /*actors.clearAndDelete();*/
    actors.clear();
    delete map;
 }
//the update function for the engine class
void Engine::update()
{    
    //create a key listener using ncurses
    int key = getch();
	
	//create switch cases for the controls
    switch (key)
    {
    //create the up case
    case UP:
        if (!map->isWall(player->y - 1, player->x))
        {
            player->y--;
            computeFov = true;
        }
        break;
		
    //create the down case
    case DOWN:
        if (!map->isWall(player->y + 1, player->x))
        {
            player->y++;
            computeFov = true;
        }
        break;

    //create the left case
    case LEFT:
        if (!map->isWall(player->y, player->x - 1))
        {
            player->x--;
            computeFov = true;
        }
        break;

	//create the right case
    case RIGHT:
        if (!map->isWall(player->y, player->x + 1))
        {
            player->x++;
            computeFov = true;
        }
    default:break;
    }
	
	//create the compute fov function
    if (computeFov)
    {
        map->computeFov();
        computeFov = false;
    }
}
//the engine render function implementation
void Engine::render()
{
    //clear the screen
    clear();
	
    // draw the map
    map->render();

    // draw the actors
    for (auto actor : actors)
    {
        if (map->isInFov(actor->x, actor->y))
        {
            actor->render();
        }
    }
}
