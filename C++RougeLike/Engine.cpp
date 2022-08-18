#include <curses.h>
#include <iostream>

#include "main.h"
#include "Colors.h"

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

    //====
	//a new Actor for the player
    player = new Actor(
        25,
        40,
        '@',
		"player",
        PLAYER_PAIR
    );	
    
	player->destructible = new PlayerDestructible(
        30,
        2,
        "your cadaver"
    );
	
    player->attacker = new Attacker(5);
    player->ai = new PlayerAi();
	
    actors.push_back(player);

	//====
	//a new map instance
    map = new Map(30, 120);
	
}

Engine::~Engine()
{
    actors.clear(); // replaces "TCODList* actors.clearAndDelete()"
    delete map; // deletes the map instance
}

//====
//the update function for the engine class
void Engine::update()
{
    //====
    // The update function must ensure the FOV is computed on first frame only.
    // This is to avoid FOV recomputation on each frame.
    if (gameStatus == STARTUP)
    {
        map->computeFov();
    }

    gameStatus = IDLE; // set the game status to idle

    player->update(); // update the player

	if (gameStatus == NEW_TURN)
	{
        for (const auto& actor : engine.actors)
        {
            if (actor != player)
            {
                actor->update(); // update monsters
            }
        }
	}
}

//====
//the engine render function implementation
void Engine::render()
{
    map->render(); // draw the map

    for (const auto& actor : actors) // iterate through the actors list
    {
        if (map->isInFov(actor->x, actor->y)) // if actor position is in the fov of the player
        {
            actor->render(); // draw the actor
        }
    }

    player->render(); // draw the player

	mvprintw(0, 0, "HP: %d", player->destructible->hp); // print the player's hp in the top left corner
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