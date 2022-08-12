#include <curses.h>
#include <iostream>
#include "Actor.h"
#include "Map.h"
#include "Engine.h"

//the constructor for the engine class
Engine::Engine() : gameStatus(STARTUP),fovRadius(10)
{
	//initialize the screen in curses
    initscr();
	//start color curses
    start_color();
    cbreak();
    noecho();
    //remove the blinking cursor
    curs_set(0);


	//make a new Actor for the player
    player = new Actor(
        25,
        40,
        '@',
		"player",
        5
    );
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
    int dx = 0, dy = 0;

    //The update function must ensure the FOV is computed on first frame only.
    // This is to avoid FOV recomputation on each frame.	
    if (gameStatus == STARTUP)
    {
        map->computeFov();
    }
    gameStatus = IDLE;
	
    //a key listener using curses to get input
    int key = getch();
	

    //move the player character
    switch (key)
    {
    case UP:
		dy = -1;
		break;
	case DOWN:
		dy = 1;
		break;
	case LEFT:
		dx = -1;
		break;
    case RIGHT:
		dx = 1;
		break;
    case QUIT:
		exit(0);
		break;
    }
	
    if (dx != 0 || dy != 0)
	{
        gameStatus = NEW_TURN;
        if (player->moveOrAttack(player->x + dx, player->y + dy))
        {
            map->computeFov();
        }
	}

	if (gameStatus == NEW_TURN)
	{
        for (const auto& actor : engine.actors)
        {
            if (actor != player)
            {
                actor->update();
            }
        }
	}

	//switch cases for the controls
 //   switch (key)
 //   {
 //   //create the up case
 //   case UP:
 //       if (!map->isWall(player->y - 1, player->x))
 //       {
 //           player->y--;
 //           computeFov = true;
 //       }
 //       break;
	//	
 //   //create the down case
 //   case DOWN:
 //       if (!map->isWall(player->y + 1, player->x))
 //       {
 //           player->y++;
 //           computeFov = true;
 //       }
 //       break;

 //   //create the left case
 //   case LEFT:
 //       if (!map->isWall(player->y, player->x - 1))
 //       {
 //           player->x--;
 //           computeFov = true;
 //       }
 //       break;

	////create the right case
 //   case RIGHT:
 //       if (!map->isWall(player->y, player->x + 1))
 //       {
 //           player->x++;
 //           computeFov = true;
 //       }
 //       break;
 //   //use the QUIT case to exit the game
 //   case QUIT:
 //       endwin();
 //       exit(0);
 //       break;
 //   }

	//
	////create the compute fov function
 //   if (computeFov)
 //   {
 //       map->computeFov();
 //       computeFov = false;
 //   }
}
//the engine render function implementation
void Engine::render()
{
    //clear the screen
    clear();
	
    // draw the map
    map->render();

    // draw the actors
    for (const auto& actor : actors)
    {
        if (map->isInFov(actor->x, actor->y))
        {
            actor->render();
        }
    }

	
}
