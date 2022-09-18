#include <iostream>
#include <curses.h>
#include <algorithm> // for std::find

#include "main.h"
#include "Colors.h"

//====
// initializes the console window, colors and other curses console functions.
// creates a new player, map, and gui.
Engine::Engine(
	int screenWidth,
	int screenHeight
) :
	gameStatus(GameStatus::STARTUP),
	fovRadius(10),
	screenWidth(screenWidth),
	screenHeight(screenHeight)	
{

	// DEBUG MESSAGE
	//std::clog << "Engine();" << std::endl;

	//==INIT_CURSES==
	initscr(); //initialize the screen in curses
	start_color(); //start color curses
	cbreak(); //disable line buffering
	noecho(); //turn off echoing of keys to the screen
	curs_set(0); //remove the blinking cursor
	keypad(stdscr, true); // enable the keypad for non-char keys
	mouse_on(ALL_MOUSE_EVENTS); // enable mouse events

	//==GUI==
	// a new Gui instance
	gui = new Gui();
}

Engine::~Engine()
{
	actors.clear(); // replaces "TCODList* actors.clearAndDelete()"
	delete map; // deletes the map instance
	delete gui; // deletes the gui instance
}

//====
// the update function to update the game logic
// and stores events
void Engine::update()
{
	
	//DEBUG log
	/*std::cout << "void Engine::update() {}" << std::endl;*/

	//==COMPUTE_FOV==
	// The update function must ensure the FOV is computed on first frame only.
	// This is to avoid FOV recomputation on each frame.
	if (gameStatus == GameStatus::STARTUP)
	{
		/*std::cout << "map->computeFov" << std::endl;*/
		map->computeFov();
	}

	gameStatus = GameStatus::IDLE; // set the game status to idle

	player->update(); // update the player

	if (gameStatus == GameStatus::NEW_TURN)
	{
		for (Actor* actor : engine.actors)
		{
			if (actor != player)
			{
				actor->update(); // update monsters
			}
		}
	}
	/*print_container(actors);*/
}

//====
// the engine render function implementation
// draws the entities on the map
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
	gui->render();
	
	//mvprintw(0, 100, "HP: %d/%d", (int)player->destructible->hp,(int)player->destructible->maxHp); // print the player's hp in the top left corner
}

//====
// earases the actor and pushes it to the begining
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
	//std::cout << "before" << std::endl;
	//std::cout << "size()->" << actors.size() << std::endl;

	// DEBUG CONTAINER
	/*print_container(actors);*/

	//erase only the actor from the list of actors
	actors.erase(std::remove(actors.begin(), actors.end(), actor), actors.end());

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

	//std::cout << "after" << std::endl;
	//std::cout << "size()->" << actors.size() << std::endl;

	//DEBUG CONTAINER
	/*print_container(actors);*/

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

//====
// prints the deque to the console window
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

//====
// This function returns the closest monster from position x, y within range.
// If range is 0, it's considered infinite.
// If no monster is found within range, it returns NULL.
Actor* Engine::getClosestMonster(int x, int y, double range) const
{
	// TODO: Add your implementation code here.
	Actor* closest = nullptr;
	float bestDistance = 1E6f;

	for (Actor* actor : engine.actors)
	{
		if (actor != player && actor->destructible && !actor->destructible->isDead())
		{
			float distance = actor->getDistance(x, y);
			if (distance < bestDistance && (distance <= range || range == 0.0f))
			{
				bestDistance = distance;
				closest = actor;
			}
		}
	}

	return nullptr;
}

//====
// The function returns a boolean to allow the player to cancel by pressing a key or right clicking.
// A range of 0 means that we allow the tile to be picked anywhere in the player's field of view.
// This function uses a default value for the maxRange parameter so that we can omit the parameter :
// engine.pickATile(&x, &y);
// is the same as
// engine.pickATile(&x, &y, 0.0f);
// We're not going to use the main loop from main.cpp while picking a tile. 
// This would require to add a flag in the engine to know if we're in standard play mode or tile picking mode.Instead, we create a alternative game loop.
// Since we want the mouse look to keep working while targetting, we need to render the game screen in the loop
bool Engine::pickATile(int* x, int* y, float maxRange)
{
	while (true)
	{
		render();
	}
	// Now the player might not be aware of where he's allowed to click. 
	// Let's highlight the zone for him. We scan the whole map and look for tiles in FOV and within range :
	// highlight the possible range
	for (int cx = 0; cx < map->map_width; cx++) 
	{
		for (int cy = 0; cy < map->map_height; cy++) 
		{
			if (
				map->isInFov(cx, cy) 
				&& 
				(
					maxRange == 0 
					||
					player->getDistance(cx, cy) <= maxRange
					)
				)
			{
				// Remember how we darkened the oldest message log by multiplying its color by a float smaller than 1 ? 
				// Well we can highlight a color using the same trick :
				/*TCODColor col = TCODConsole::root->getCharBackground(cx, cy);*/
				/*col = col * 1.2f;*/
				/*TCODConsole::root->setCharBackground(cx, cy, col);*/
				mvchgat(cy, cx, 1, A_NORMAL, HPBARFULL_PAIR, NULL);
			}
		}

		// Now we need to update the mouse coordinate in Engine::mouse, so let's duplicate the checkForEvent call from Engine::update :

			/*TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS | TCOD_EVENT_MOUSE, &lastKey, &mouse);*/
		
		request_mouse_pos();
		// We're going to do one more thing to help the player select his tile : fill the tile under the mouse cursor with white :
		if (
			map->isInFov(Mouse_status.x, Mouse_status.y)
			&&
			(
				maxRange == 0 
				|| 
				player->getDistance(Mouse_status.x, Mouse_status.y) <= maxRange
				)
			) 
		{
			/*TCODConsole::root->setCharBackground(mouse.cx, mouse.cy, TCODColor::white);*/
			mvchgat(Mouse_status.y, Mouse_status.x, 1, A_NORMAL, HPBARMISSING_PAIR, NULL);
			// And if the player presses the left button while a valid tile is selected, return the tile coordinates :

			//if (Mouse_status.button) {
			//	*x = mouse.cx;
			//	*y = mouse.cy;
			//	return true;
			//}
		}
		
		
	}

	return false;
}

//====
// When the Engine is created, 
// we don't know yet if we have to generate a new map or load a previously saved one.
// Will be called in load()
void Engine::init()
{
	//====
	// a new Actor for the player
	player = new Actor(
		25,
		40,
		'@',
		"Steven Seagull",
		PLAYER_PAIR
	);

	player->destructible = new PlayerDestructible(
		30,
		2,
		"your cadaver"
	);

	player->attacker = new Attacker(100);
	player->ai = new PlayerAi();
	player->container = new Container(26);
	actors.push_back(player);

	//====
	// a new Map instance
	map = new Map(30 - 8, 120); // need to make space for the gui (-7y)
	map->init(true);

}

void Engine::load()
{
	if (TCODSystem::fileExists("game.sav"))
	{
		TCODZip zip;

		zip.loadFromFile("game.sav");

		// load the map
		int width = zip.getInt();
		int height = zip.getInt();
		map = new Map(height, width);
		map->load(zip);

		// load the player
		player = new Actor(0, 0, 0, "dummy name", EMPTY_PAIR);
		player->load(zip);
		actors.push_back(player);

		// then all other actors
		int nbActors = zip.getInt();
		while (nbActors > 0)
		{
			Actor* actor = new Actor(0, 0, 0, "dummy name", EMPTY_PAIR);
			actor->load(zip);
			actors.push_back(actor);
			nbActors--;
		}

		// finally the message log
		gui->load(zip);
	}
	else
	{
		engine.init();
	}
}

void Engine::save()
{
	// handle the permadeath
	// delete the save file if the player is dead
	if (player->destructible->isDead())
	{
		TCODSystem::deleteFile("game.sav");
	}
	// else save the map
	else
	{
		std::clog << "saving..." << std::endl;

		TCODZip zip;

		// save the map first
		zip.putInt(map->map_width);
		zip.putInt(map->map_height);
		map->save(zip);

		// then save the player
		player->save(zip);

		// then all the other actors
		zip.putInt(actors.size() - 1);
		for (Actor* actor : actors)
		{
			if (actor != player)
			{
				actor->save(zip);
			}
		}

	// save the message log
	gui->save(zip);

	zip.saveToFile("game.sav");
	}
	
}
