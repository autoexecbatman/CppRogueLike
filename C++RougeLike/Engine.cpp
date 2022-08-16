#include <curses.h>
#include <iostream>

#include "main.h"

//====

Engine::Engine(
    int screenWidth,
    int screenHeight
) : 
    gameStatus(STARTUP),
    fovRadius(10),
	screenWidth(screenWidth),
	screenHeight(screenHeight)
{
    initscr(); //initialize the screen in curses
	start_color(); //start color curses
    cbreak(); //disable line buffering
    noecho(); //turn off echoing of keys to the screen
    curs_set(0); //remove the blinking cursor


	//a new Actor for the player
    player = new Actor(
        25,
        40,
        '@',
		"player",
        5
    );	
    
	player->destructible = new PlayerDestructible(
        30,
        2,
        "your cadaver"
    );
	
    player->attacker = new Attacker(5);
    player->ai = new PlayerAi();
	
    actors.push_back(player);

	//a new map instance
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
    //The update function must ensure the FOV is computed on first frame only.
    // This is to avoid FOV recomputation on each frame.
    if (gameStatus == STARTUP)
    {
        map->computeFov();
    }
    gameStatus = IDLE;//set the game status to idle
    player->update();
    ////a key listener using curses to get input
    //int key = getch();
    ////the clear function should be called before each update
    //clear();
	
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
	
	
 //   //move the player character
 //   switch (key)
 //   {
 //   case UP:
	//	dy = -1;
	//	break;
	//case DOWN:
	//	dy = 1;
	//	break;
	//case LEFT:
	//	dx = -1;
	//	break;
 //   case RIGHT:
	//	dx = 1;
	//	break;
 //   case QUIT:
	//	exit(0);
	//	break;
 //   }
	
 //   if (dx != 0 || dy != 0)
	//{
 //       gameStatus = NEW_TURN;
 //       if (player->moveOrAttack(player->x + dx, player->y + dy))
 //       {
 //           map->computeFov();
 //       }
	//}
    
}
//the engine render function implementation
void Engine::render()
{
    //clear the screen
    //clear();

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

    player->render();
	//show the player's stats in curses
	mvprintw(0, 0, "HP: %d", player->destructible->hp);
}

void Engine::sendToBack(Actor* actor)
{
	//TODO : Find the right member function to remove actor from vector
	
    //actors.remove(actor);
    //actors.insertBefore(actor,0);

	//removes the actor from the vector using std::vector erase function
	actors.erase(std::find(actors.begin(), actors.end(), actor), actors.end());
	//adds the actor to the vector using std::vector insert function
	actors.insert(actors.begin(), actor);
}
