#include <iostream>
#include <curses.h>
#include <algorithm> // for std::remove in sendToBack(Actor*)
#include <random>

#include "Engine.h"
#include "Actor.h"
#include "Map.h"
#include "Gui.h"
#include "Destructible.h"
#include "Attacker.h"
#include "Ai.h"
#include "Pickable.h"
#include "Container.h"


#include "Colors.h"
#include "Window.h"

//==ENGINE_CONSTRUCTOR==
// initializes the console window, colors and other curses console functions.
// creates a new player, map, and gui.
Engine::Engine(
	Length screenWidth,
	Length screenHeight
) :
	gameStatus(GameStatus::STARTUP),
	fovRadius(10),
	screenWidth(screenWidth),
	screenHeight(screenHeight),
	player(nullptr),
	stairs(nullptr),
	map(nullptr),
	gui(nullptr),
	run(true),
	keyPress(0)
{

	// DEBUG MESSAGE
	//std::clog << "Engine();" << std::endl;

	//==INIT_CURSES==
	initscr(); // initialize the screen in curses
	start_color(); // start color curses
	cbreak(); // disable line buffering
	noecho(); // turn off echoing of keys to the screen
	curs_set(0); // remove the blinking cursor
	keypad(stdscr, true); // enable the keypad for non-char keys
	mouse_on(ALL_MOUSE_EVENTS); // enable mouse events

	//==GUI==
	// a new Gui instance
	gui = new Gui();
	level = 0;
}

Engine::~Engine()
{
	actors.clear(); // clear the actors deque

	if (map) delete map; // delete the map

	gui->gui_clear(); // clears the message log

	delete gui; // deletes the gui instance
}

//====
// When the Engine is created, 
// we don't know yet if we have to generate a new map or load a previously saved one.
// Will be called in load()
void Engine::init()
{
	//==PLAYER==
	// a new Actor for the player
	player = new Actor(
		25,
		40,
		'@',
		"Steven Seagull",
		PLAYER_PAIR
	);

	player->destructible = new PlayerDestructible(
		10 + random_number(1,10),
		10,
		"your cadaver",
		0
	);

	player->attacker = new Attacker(random_number(1,10));
	player->ai = new PlayerAi();
	player->container = new Container(26);
	actors.push_back(player);

	//==STAIRS==
	// create stairs for player to descend to the next level
	stairs = new Actor(
		0,
		0,
		'>',
		"stairs",
		WHITE_PAIR
	);
	stairs->blocks = false;
	stairs->fovOnly = false;
	actors.push_back(stairs);

	//==MAP==
	// a new Map instance
	map = new Map(30 - 8, 120); // need to make space for the gui (-7y)
	map->init(true);
	gameStatus = GameStatus::STARTUP;
}

//==ENGINE_UPDATE==
// the update function to update the game logic
// and stores events
void Engine::update()
{
	//==COMPUTE_FOV==
	// The update function must ensure the FOV is computed on first frame only.
	// This is to avoid FOV recomputation on each frame.
	if (gameStatus == GameStatus::STARTUP)
	{
		map->compute_fov();
	}

	gameStatus = GameStatus::IDLE; // set the game status to idle

	player->update(); // update the player

	if (gameStatus == GameStatus::NEW_TURN) // check new turn
	{
		for (Actor* actor : actors) // go through the list of actors
		{
			if (actor != player) // check if the actor is not the player
			{
				actor->update(); // update all actors except the player
			}
		}
	}
}

//====
// the engine render function implementation
// draws the entities on the map
void Engine::render()
{
	map->render(); // draw the map

	for (Actor* actor : actors) // iterate through the actors list
	{
		if (actor != player // check if the actor is not the player
			&&
			(
				(
					!actor->fovOnly
					&&
					map->is_explored(actor->posX,actor->posY)
					) // check if the actor is not fovOnly and is explored
			||
			map->is_in_fov(actor->posX, actor->posY)
				) // OR if the actors position is in the FOV of the player
			) // end of if statement
		{
			actor->render(); // draw the actor
		}
	}

	player->render(); // draw the player
	gui->render(); // draw the gui
}

//====
// earases the actor and pushes it to the begining
void Engine::send_to_back(Actor* actor)
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

	actors.push_front(actor);

	//std::cout << "after" << std::endl;
	//std::cout << "size()->" << actors.size() << std::endl;

	//DEBUG CONTAINER
	/*print_container(actors);*/
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
Actor* Engine::get_closest_monster(int fromPosX, int fromPosY, double inRange) const
{
	// TODO: Add your implementation code here.
	Actor* closestMonster = nullptr;
	float bestDistance = 1E6f;

	for (Actor* actor : engine.actors)
	{
		if (actor != player && actor->destructible && !actor->destructible->is_dead())
		{
			float distance = actor->get_distance(fromPosX, fromPosY);
			if (distance < bestDistance && (distance <= inRange || inRange == 0.0f))
			{
				bestDistance = distance;
				closestMonster = actor;
			}
		}
	}

	return closestMonster;
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
bool Engine::pick_tile(int* x, int* y, float maxRange)
{
	//while (engine.run == true)
	//{
	//	clear();
	//	render();
	//	
	//	nodelay(stdscr, true);
	//// Now the player might not be aware of where he's allowed to click. 
	//// Let's highlight the zone for him. We scan the whole map and look for tiles in FOV and within range :
	//// highlight the possible range
	//	for (int cx = 0; cx < map->map_width; cx++)
	//	{
	//		for (int cy = 0; cy < map->map_height; cy++)
	//		{
	//			if (
	//				map->isInFov(cx, cy)
	//				&&
	//				(
	//					maxRange == 0
	//					||
	//					player->getDistance(cx, cy) <= maxRange
	//					)
	//				)
	//			{
	//				// Remember how we darkened the oldest message log by multiplying its color by a float smaller than 1 ? 
	//				// Well we can highlight a color using the same trick :
	//				/*TCODColor col = TCODConsole::root->getCharBackground(cx, cy);*/
	//				/*col = col * 1.2f;*/
	//				/*TCODConsole::root->setCharBackground(cx, cy, col);*/
	//				/*mvchgat(cy, cx, 1, A_NORMAL, HPBARFULL_PAIR, NULL);*/
	//				attron(COLOR_PAIR(DARK_GROUND_PAIR));
	//				mvwprintw(stdscr, cy, cx, "0");
	//				attroff(COLOR_PAIR(DARK_GROUND_PAIR));
	//				refresh();
	//			}
	//		}
	//	}
	//	nodelay(stdscr, false);
	//	// Now we need to update the mouse coordinate in Engine::mouse, so let's duplicate the checkForEvent call from Engine::update :
	//		/*TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS | TCOD_EVENT_MOUSE, &lastKey, &mouse);*/
	//	request_mouse_pos();
	//		// We're going to do one more thing to help the player select his tile : fill the tile under the mouse cursor with white :
	//	if (
	//		map->isInFov(Mouse_status.x, Mouse_status.y)
	//		&&
	//		(
	//			maxRange == 0
	//			||
	//			player->getDistance(Mouse_status.x, Mouse_status.y) <= maxRange
	//			)
	//		)
	//	{
	//		/*TCODConsole::root->setCharBackground(mouse.cx, mouse.cy, TCODColor::white);*/
	//		mvchgat(Mouse_status.y, Mouse_status.x, 1, A_NORMAL, HPBARMISSING_PAIR, NULL);
	//		attron(COLOR_PAIR(HPBARMISSING_PAIR));
	//		mvprintw(Mouse_status.y, Mouse_status.x, "X");
	//		attroff(COLOR_PAIR(HPBARMISSING_PAIR));
	//		refresh();
	//		
	//		// And if the player presses the left button while a valid tile is selected, return the tile coordinates :
	//		if (BUTTON1_CLICKED)
	//		{
	//			// first display a message
	//			gui->log_message(WHITE_PAIR, "Target confirmed");
	//			// then return the coordinates
	//			
	//			/*request_mouse_pos();*/
	//			*x = Mouse_status.x;
	//			*y = Mouse_status.y;
	//			return true;
	//		}
	//	}
	//	// If the player pressed a key or right clicked, we exit :
	//	if (BUTTON3_CLICKED)
	//	{
	//		return false;
	//	}
	//	refresh();
	//}


	int targetCursorY = player->posY; // init position Y
	int targetCursorX = player->posX; // init position X

	int lastY = targetCursorY;
	int lastX = targetCursorX;

	int lineY = 0;
	int lineX = 0;

	bool run = true;


	while (run == true)
	{
		clear();

		// make the line follow the mouse position
		// if mouse move
		if (mouse_moved())
		{
			request_mouse_pos();
			targetCursorY = Mouse_status.y;
			targetCursorX = Mouse_status.x;
		}
		engine.render();

		//display the FOV in white

		for (int tilePosX = 0; tilePosX < engine.map->map_width; tilePosX++)
		{
			for (int tilePosY = 0; tilePosY < engine.map->map_height; tilePosY++)
			{
				if (engine.map->is_in_fov(tilePosX, tilePosY))
				{
					mvchgat(tilePosY, tilePosX, 1, A_REVERSE, LIGHTNING_PAIR, NULL);
				}
			}
		}

		// first color the player position if the cursor has moved from the player position
		if (targetCursorY != player->posY || targetCursorX != player->posX)
		{
			mvchgat(lastY, lastX, 1, A_NORMAL, WHITE_PAIR, NULL);
		}

		// draw a line using TCODLine class
		/*
		@CppEx
		// Going from point 5,8 to point 13,4
		int x = 5, y = 8;
		TCODLine::init(x, y, 13, 4);
		do {
			// update cell x,y
		} while (!TCODLine::step(&x, &y));
		*/
		TCODLine::init(player->posX, player->posY, targetCursorX, targetCursorY);
		while (!TCODLine::step(&lineX, &lineY))
		{
			mvchgat(lineY, lineX, 1, A_STANDOUT, WHITE_PAIR, NULL);
		}

		// the player uses the keyboard to select a target
		// the target selection cursor is displayed in white
		// the player can press enter to select the target
		// or press escape to cancel the target selection
		// 'X' is the char for the selection cursor in the target selection mode

		attron(COLOR_PAIR(HPBARMISSING_PAIR));
		mvaddch(targetCursorY, targetCursorX, 'X');
		attroff(COLOR_PAIR(HPBARMISSING_PAIR));

		// if the cursor is on a monster then display the monster's name

		if (engine.map->is_in_fov(targetCursorX, targetCursorY))
		{
			Actor* actor = engine.map->get_actor(targetCursorX, targetCursorY);
			// and actor is not an item
			if (actor != nullptr && actor->destructible != nullptr)
			{
				mvprintw(0, 0, actor->name);
				// print the monster's stats
				mvprintw(1, 0, "HP: %d/%d", static_cast<int>(actor->destructible->hp), static_cast<int>(actor->destructible->hpMax));
				mvprintw(2, 0, "AC: %d", actor->destructible->defense);
			}
		}

		refresh();

		// get the key press
		int key = getch();
		switch (key)
		{
		case KEY_UP:
			// move the selection cursor up
			targetCursorY--;
			break;

		case KEY_DOWN:
			// move the selection cursor down
			targetCursorY++;
			break;

		case KEY_LEFT:
			// move the selection cursor left
			targetCursorX--;
			break;

		case KEY_RIGHT:
			// move the selection cursor right
			targetCursorX++;
			break;

		case 'f':
			// if the player presses the 'f' key then the target selection is confirmed
			// and the target coordinates are returned
			
			// first display a message
			gui->log_message(WHITE_PAIR, "Target confirmed");
			// then return the coordinates
			*x = targetCursorX;
			*y = targetCursorY;

			return true;
			break;

		case 10:


			// if the key enter is pressed then select the target
			// and return the target position

			// if the target is a monster then attack it
		{
			if (engine.map->is_in_fov(targetCursorX, targetCursorY))
			{
				Actor* actor = engine.map->get_actor(targetCursorX, targetCursorY);
				// and actor is not an item
				if (actor != nullptr && actor->destructible != nullptr)
				{
					player->attacker->attack(player, actor);
					run = false;
				}
			}
		}
		break;
		case 'r':
		case 27:
			// if the key escape is pressed then cancel the target selection
			run = false;
			break;

		default:break;
		}


	}
	clear();


	return false;
}


void Engine::game_menu()
{
	// this function will contain a loop where the game menu will be displayed
	// and the player will be able to choose an option
	// the function will return when the player chooses an option
	
	// first we assign the variables we need for the function

	bool menu_run = true; // this variable will be used to keep the loop running
	int menu_key = 0; // this variable will contain the key the player presses
	int new_option = 0;
	int old_option = -1;

	// we create a new window for the menu
	WINDOW* menu_win = newwin(
		6, // int nlines
		20, // int ncols
		10, // int begy
		40 // int begx
	);
	box(menu_win, 0, 0);
	int max_x = getmaxx(menu_win); // get the maximum x value of the window
	int max_y = getmaxy(menu_win); // get the maximum y value of the window
	
	refresh();
	
	while (menu_run == true) 
	{
		// first clear the window
		wclear(menu_win);

		// then draw the menu

		// first print the menu title

		mvwprintw(menu_win, 0, 4, "==THE MENU==");

		// then print the menu selection options

		mvwprintw(menu_win, 1, 4, "[N,n] New Game");
		mvwprintw(menu_win, 2, 4, "[L,l] Load");
		mvwprintw(menu_win, 3, 4, "[Q,q] Quit");

		// then print the menu cursor

		mvwprintw(menu_win, new_option + 1, 1, ">");
		mvwprintw(menu_win, old_option + 1, 1, " ");
		old_option = new_option;

		wrefresh(menu_win);

		// get the key press
		menu_key = getch();
		switch (menu_key)
		{
		case KEY_UP:
			// move the selection cursor up
			new_option--;
			if (new_option < 0) new_option = 2;
			break;

		case KEY_DOWN:
			// move the selection cursor down
			new_option++;
			if (new_option > 2) new_option = 0;
			break;

		case 'Q':
		case 'q':
			// quit the menu window
			exit(0);
			break;

		case 'N':
		case 'n':
			// if the key enter is pressed and cursor is on "New Game" then start a new game
			menu_run = false;
			delwin(menu_win);
			engine.init();
			break;

		case 'L':
		case 'l':
			// if the key enter is pressed and cursor is on "Continue" then load the game
			menu_run = false;
			delwin(menu_win);
			engine.load();
			break;

		default:break;
		}
	}
}

bool Engine::mouse_moved()
{
	int old_mouse_x = Mouse_status.x;
	int old_mouse_y = Mouse_status.y;

	// check if the mouse has moved
	
	// first we get the current mouse position
	request_mouse_pos();
	
	// then we compare it to the previous mouse position
	if (Mouse_status.x != old_mouse_x || Mouse_status.y != old_mouse_y)
	{
		// if the mouse has moved, we update the old mouse position
		old_mouse_x = Mouse_status.x;
		old_mouse_y = Mouse_status.y;
		return true;
	}
	else
	{
		return false;
	}

}

void Engine::target()
{
	int targetCursorY = player->posY; // init position Y
	int targetCursorX = player->posX; // init position X

	int lastY = targetCursorY;
	int lastX = targetCursorX;

	int lineY = 0;
	int lineX = 0;

	bool run = true;


	while (run == true)
	{
		clear();

		// make the line follow the mouse position
		// if mouse move
		if (mouse_moved())
		{
			request_mouse_pos();
			targetCursorY = Mouse_status.y;
			targetCursorX = Mouse_status.x;
		}
		engine.render();

		//display the FOV in white
		
		for (int tilePosX = 0; tilePosX < engine.map->map_width; tilePosX++)
		{
			for (int tilePosY = 0; tilePosY < engine.map->map_height; tilePosY++)
			{
				if (engine.map->is_in_fov(tilePosX, tilePosY))
				{
					mvchgat(tilePosY, tilePosX, 1, A_REVERSE, LIGHTNING_PAIR, NULL);
				}
			}
		}

		// first color the player position if the cursor has moved from the player position
		if (targetCursorY != player->posY || targetCursorX != player->posX)
		{
			mvchgat(lastY, lastX, 1, A_NORMAL, WHITE_PAIR, NULL);
		}

		// draw a line using TCODLine class
		/*
		@CppEx
		// Going from point 5,8 to point 13,4
		int x = 5, y = 8;
		TCODLine::init(x, y, 13, 4);
		do {
			// update cell x,y
		} while (!TCODLine::step(&x, &y));
		*/
		TCODLine::init(player->posX, player->posY, targetCursorX, targetCursorY);
		while (!TCODLine::step(&lineX, &lineY))
		{
			mvchgat(lineY, lineX, 1, A_STANDOUT, WHITE_PAIR, NULL);
		}
		
		// the player uses the keyboard to select a target
		// the target selection cursor is displayed in white
		// the player can press enter to select the target
		// or press escape to cancel the target selection
		// 'X' is the char for the selection cursor in the target selection mode

		attron(COLOR_PAIR(HPBARMISSING_PAIR));
		mvaddch(targetCursorY,targetCursorX,'X');
		attroff(COLOR_PAIR(HPBARMISSING_PAIR));

		// if the cursor is on a monster then display the monster's name
		
		if (engine.map->is_in_fov(targetCursorX, targetCursorY))
		{
			Actor* actor = engine.map->get_actor(targetCursorX, targetCursorY);
			// and actor is not an item
			if (actor != nullptr && actor->destructible != nullptr )
			{
				mvprintw(0, 0, actor->name);
				// print the monster's stats
				mvprintw(1, 0, "HP: %d/%d", static_cast<int>(actor->destructible->hp), static_cast<int>(actor->destructible->hpMax));
				mvprintw(2, 0, "AC: %d", actor->destructible->defense);
			}
		}
		
		refresh();

		// get the key press
		int key = getch();
		switch (key)
		{
		case KEY_UP:
			// move the selection cursor up
			targetCursorY--;
			break;

		case KEY_DOWN:
			// move the selection cursor down
			targetCursorY++;
			break;

		case KEY_LEFT:
			// move the selection cursor left
			targetCursorX--;
			break;

		case KEY_RIGHT:
			// move the selection cursor right
			targetCursorX++;
			break;
			
		case 10:
			
			// if the key enter is pressed then select the target
			// and return the target position
			
			// if the target is a monster then attack it
		{
			if (engine.map->is_in_fov(targetCursorX, targetCursorY))
			{
				Actor* actor = engine.map->get_actor(targetCursorX, targetCursorY);
				// and actor is not an item
				if (actor != nullptr && actor->destructible != nullptr)
				{
					player->attacker->attack(player,actor);
					run = false;
				}
			}
		}
			break;
		case 'r':
		case 27:
			// if the key escape is pressed then cancel the target selection
			run = false;
			break;
			
		default:break;
		}


	}
	clear();
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
		player = new Actor(0, 0, 0, "loaded player", EMPTY_PAIR);
		player->load(zip);
		actors.push_back(player);

		// load the stairs
		stairs = new Actor(0, 0, 0, "loaded stairs", WHITE_PAIR);
		stairs->load(zip);
		actors.push_back(stairs);

		// then all other actors
		int nbActors = zip.getInt();
		while (nbActors > 0)
		{
			Actor* actor = new Actor(0, 0, 0, "loaded other actors", EMPTY_PAIR);
			actor->load(zip);
			actors.push_back(actor);
			nbActors--;
		}

		// finally the message log
		gui->load(zip);
	}
	// TODO : Delete this line moved to switch case in game_menu
	else
	{
		engine.init();
	}
}

void Engine::save()
{
	// handle the permadeath
	// delete the save file if the player is dead
	if (player->destructible->is_dead())
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

		// then save the stairs
		stairs->save(zip);

		// then all the other actors
		zip.putInt(actors.size() - 2);
		for (Actor* actor : actors)
		{
			if (actor != player && actor != stairs)
			{
				actor->save(zip);
			}
		}

	// save the message log
	gui->save(zip);

	zip.saveToFile("game.sav");
	}
	
}

void Engine::term()
{
	actors.clear();
	if (map) delete map;
	gui->gui_clear();
}

void Engine::next_level()
{
	level++;
	gui->log_message(WHITE_PAIR, "You take a moment to rest, and recover your strength.");
	player->destructible->heal(player->destructible->hpMax / 2);
	gui->log_message(WHITE_PAIR, "After a rare moment of peace, you descend\ndeeper into the heart of the dungeon...");
	gui->log_message(WHITE_PAIR, "You are now on level %d", level);

	// clear the dungeon except the player and the stairs
	actors.clear();
	actors.push_back(player);
	actors.push_back(stairs);
	
	print_container(actors);
	
	delete map;


	
	map = new Map(30 - 8, 120);
	map->init(true);
	gameStatus = GameStatus::STARTUP;

	
}

// create the getActor function
Actor* Engine::get_actor(int x, int y) const
{
	for (Actor* actor : actors)
	{
		if (actor->posX == x && actor->posY == y)
		{
			return actor;
		}
	}
	return nullptr;
}

void Engine::dispay_stats(int level)
{
	// TODO: Add your implementation code here.
	// display the player stats
	WINDOW* stats = newwin(
		11, // height
		30, // width
		0, // y
		0 // x
	);
	
	box(stats, 0, 0);
	refresh();
	while (true)
	{
		mvwprintw(stats, 1, 1, "Player Stats");
		mvwprintw(stats, 2, 1, "Level: %d", level);
		mvwprintw(stats, 3, 1, "Experience: %d", player->destructible->xp);
		mvwprintw(stats, 4, 1, "Food: %d/%d", player->destructible->food, player->destructible->foodMax);
		mvwprintw(stats, 5, 1, "Need to sleep: %d", player->destructible->needToSleep);
		mvwprintw(stats, 6, 1, "[a] Attack: %d", player->attacker->power);
		mvwprintw(stats, 7, 1, "[d] Defense: %d", player->destructible->defense);
		mvwprintw(stats, 8, 1, "[h] Health: %d/%d", player->destructible->hp, player->destructible->hpMax);
		
		wrefresh(stats);

		int key = getch();
		switch (key)
		{
		case 'a':
			player->attacker->power += 1;
			return;
		case 'd':
			player->destructible->defense += 1;
			return;
		case 'h':
			player->destructible->hpMax += 1;
			return;
		}
	}
}

// display character sheet
void Engine::display_character_sheet()
{
	WINDOW* character_sheet = newwin(
		30, // height
		120, // width
		0, // y
		0 // x
	);

	box(character_sheet, 0, 0);

	refresh();
	// display the player stats
	bool run = true;
	while (run == true)
	{
		// display the player stats
		// using a dnd character sheet
		// based on https://wiki.roll20.net/ADnD_2nd_Edition_Character_sheet

		// display the player name
		mvwprintw(character_sheet, 1, 1, "Name: ");
		// display the player class
		mvwprintw(character_sheet, 2, 1, "Class: ");
		// display the class kit
		mvwprintw(character_sheet, 3, 1, "Kit: ");
		// display the player level
		mvwprintw(character_sheet, 4, 1, "Level: %d", level);
		// display the player experience
		mvwprintw(character_sheet, 5, 1, "Experience: %d", player->destructible->xp);
		// display the player alignment
		mvwprintw(character_sheet, 6, 1, "Alignment: ");
		// add character details on the right side
		// display the player race
		mvwprintw(character_sheet, 1, 60, "Race: ");
		// display gender
		mvwprintw(character_sheet, 2, 60, "Gender: ");
		// display hair color
		mvwprintw(character_sheet, 3, 60, "Hair Color: ");
		// display eye color
		mvwprintw(character_sheet, 4, 60, "Eye Color: ");
		// display complexion
		mvwprintw(character_sheet, 5, 60, "Complexion: ");
		// display features 
		mvwprintw(character_sheet, 6, 60, "Features: ");
		// display homeland
		mvwprintw(character_sheet, 7, 60, "Homeland: ");
		// display deity
		mvwprintw(character_sheet, 8, 60, "Deity: ");
		// display vision
		mvwprintw(character_sheet, 9, 60, "Vision: ");
		// display secondary skills
		mvwprintw(character_sheet, 10, 60, "Secondary Skills: ");

		wrefresh(character_sheet);

		int key = getch();
		// if any key was pressed then exit the loop
		if (key != ERR)
		{
			run = false;
		}
	}
	clear();
}

// make a random number function to use in the game
int Engine::random_number(int min, int max)
{
	static std::default_random_engine randomEngine(time(nullptr));
	std::uniform_int_distribution<int> range(min, max);
	
	return range(randomEngine);
}

// displays the actors names
void Engine::wizard_eye()
{
	for (const auto& actor : engine.actors)
	{
		// print the actor's name
		mvprintw(actor->posY, actor->posX, actor->name);
	}
}