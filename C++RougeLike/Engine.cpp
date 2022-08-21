#include <curses.h>
#include <iostream>
#include <algorithm>

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

	mvprintw(0, 100, "HP: %d/%d", (int)player->destructible->hp,(int)player->destructible->maxHp); // print the player's hp in the top left corner
}

void Engine::sendToBack(Actor* actor)
{
	//TODO : Find the right member function to remove actor from vector
	//Since actors are drawn in their order in the list,
	// a corpse my be drawn on top of a living actor.
	//To keep that from happening, 
	//we simply move the dead actors to the beginning of the list
	//in the Engine::sendToBack function
	//example: 
	// actors.remove(actor); // remove by value
	// 
	// 	void remove(const T elt) {
	//    for (T* curElt = begin(); curElt != end(); curElt++) {
	//        if (*curElt == elt) {
	//            remove(curElt);
	//            return;
	//        }
	//    }
	//}
	// 
	//example: 
	// actors.insertBefore(actor,0);
	// 	T * insertBefore(const T elt,int before) {
	//    if (fillSize + 1 >= allocSize) allocate();
	//    for (int idx = fillSize; idx > before; idx--) {
	//        array[idx] = array[idx - 1];
	//    }
	//    array[before] = elt;
	//    fillSize++;
	//    return &array[before];
	//}
	//removes the actor from the vector using std::deque erase function
	std::cout << "before" << std::endl;
	std::cout << "size()->" << actors.size() << std::endl;
	print_container(actors);

	//erase only the actor from the list of actors
	/*actors.erase(std::remove(actors.begin(), actors.end(), actor), actors.end());*/
	std::remove(actors.begin(), actors.end(), actor);

	//actors.erase( // erases elements
 //       std::find( // 
 //           actors.begin(),
 //           actors.end(),
 //           actor
 //       ),// first iterator
	//	actors.end() // last iterator
 //   );

	//adds the actor to the vector using std::vector insert function
	/*actors.insert(actors.begin(), actor);*/
	actors.push_front(actor);

	std::cout << "after" << std::endl;
	std::cout << "size()->" << actors.size() << std::endl;
	print_container(actors);

	////send destructible to back of the list
	//auto it = actor;
	//for (auto it = actors.begin(); it != actors.end();)
	//{
	//    if (*it)
	//    {
	//        it = actors.erase(it);
	//    }
	//    else
	//    {
	//        ++it;
	//    }
	//}

	//actors.push_front(it);

}

void Engine::print_container(const std::deque<Actor*> actors)
{
	int i = 0;
	for (const auto& actor : actors)
	{
		std::cout << actor->name << i << " ";
		i++;
	}
	std::cout << '\n';
}