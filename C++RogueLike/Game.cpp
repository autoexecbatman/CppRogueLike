// file: Game.cpp
#include <iostream>
#include <curses.h>
#include <algorithm> // for std::remove in sendToBack(Actor*)
#include <random>
#include <climits>
#include <cassert>
#include <gsl/util>

#include "Game.h"
#include "Actor.h"
#include "Map.h"
#include "Gui.h"
#include "Attacker.h"
#include "Ai.h"
#include "AiPlayer.h"
#include "Pickable.h"
#include "Container.h"
#include "Colors.h"
#include "Window.h"
#include "RandomDice.h"
#include "Player.h"

//====
// When the Game is created, 
// we don't know yet if we have to generate a new map or load a previously saved one.
// Will be called in load()
void Game::init()
{
	create_player();

	//==STAIRS==
	game.stairs->blocks = false;
	game.stairs->fovOnly = false;
	
	game.actors.push_back(stairs);

	//==MAP==
	game.map->init(true); // set the checker function

	gameStatus = GameStatus::STARTUP;
	std::clog << "GameStatus::STARTUP" << std::endl;
}

void Game::create_player()
{
	//==PLAYER==
	RandomDice d;
	int playerHp = 20 + d.d10();
	int playerDamage = 2 + d.d8();

	game.player = std::make_shared<Player>(0, 0, playerHp, 5, "your cadaver", 0, playerDamage, true);
	game.actors.emplace_back(game.player);
}

//==ENGINE_UPDATE==
// the update function to update the game logic
// and stores events
void Game::update()
{
	if (game.player && game.player->destructible)
	{
		if (game.player->destructible->is_dead())
		{
			std::clog << "Player is dead!" << std::endl;
			run = false;
			game.gui->log_message(COLOR_RED, "You died!\nPress any key to exit.");
		}
		else
		{
			if (Game::gameStatus == GameStatus::STARTUP)
			{
				std::clog << "...Computing FOV..." << std::endl;
				game.map->compute_fov();
			}

			gameStatus = GameStatus::IDLE;
			std::clog << "GameStatus::IDLE" << std::endl;

			std::clog << "Updating player..." << std::endl;
			game.player->update();
			std::clog << "Player updated!" << std::endl;

			std::clog << "Updating actors..." << std::endl;
			if (Game::gameStatus == GameStatus::NEW_TURN)
			{
				for (const auto& actor : actors)
				{
					if (actor && actor != player)
					{
						actor->update();
					}
				}
			}
			std::clog << "Actors updated!" << std::endl;
		}
	}
	else
	{
		std::clog << "Error: Game::update() - game.player or game.player->destructible is null" << std::endl;
		std::cout << "Error: Game::update() - game.player or game.player->destructible is null" << std::endl;
		exit(-1);
	}
}

//====
// the engine render function implementation
// draws the entities on the map
void Game::render()
{
	map->render();

	std::clog << "Actors are trying to be drawn..." << std::endl;

	for (const auto& actor : actors)
	{
		if (actor && actor != player)
		{
			const bool isVisible = (!actor->fovOnly && map->is_explored(actor->posX, actor->posY))
				|| map->is_in_fov(actor->posX, actor->posY);

			if (isVisible)
			{
				std::clog << "Actor: " << actor->name << " is in FOV" << std::endl;
				actor->render();
				std::clog << "Actor: " << actor->name << " is drawn" << std::endl;
			}
		}
	}
	std::clog << "Actors are drawn" << std::endl;

	std::clog << "Player is trying render..." << std::endl;
	player->render();
	std::clog << "Player is drawn" << std::endl;

	std::clog << "GUI is trying render..." << std::endl;
	//gui->render();
	std::clog << "GUI is drawn" << std::endl;

	std::clog << "GUI Refreshed" << std::endl;
	std::clog << "GUI Rendered" << std::endl;
	std::clog << "RENDER FUNCTION OUT" << std::endl;
}

void Game::send_to_back(Actor& actor)
{
	for (const auto& a : actors) // loop through the actors
	{
		if (a.get() == &actor) // if the actor is found
		{
			auto it = std::find_if(actors.begin(), actors.end(), [&actor](const auto& a) noexcept { return a.get() == &actor; }); // get the iterator of the actor
			const auto distance = std::distance(actors.begin(), it); // get the distance from the begining of the vector to the actor
			for (auto i = distance; i > 0; i--)
			{
				std::swap(gsl::at(actors, i - 1), gsl::at(actors, i));
			}
		}
	}
}

std::shared_ptr<Actor> Game::get_closest_monster(int fromPosX, int fromPosY, double inRange) const
{
	std::shared_ptr<Actor> closestMonster = nullptr;
	int bestDistance = INT_MAX;

	for (auto actor = actors.begin(); actor != actors.end(); ++actor)
	{
		if (*actor != player
			&&
			(*actor)->destructible
			&&
			!(*actor)->destructible->is_dead()
			)
		{
			int distance = (*actor)->get_distance(fromPosX, fromPosY);
			if (distance < bestDistance && (distance <= inRange || inRange == 0.0f))
			{
				bestDistance = distance;
				closestMonster = *actor;
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
bool Game::pick_tile(int* x, int* y, int maxRange)
{
	//while (game.run == true)
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

	int sideLength = 5;

	int height = sideLength;
	int width = sideLength;

	// Create the window once
	WINDOW* aoe = newwin(height + 2, width + 2, 0, 0);
	box(aoe, 0, 0);
	wbkgd(aoe, COLOR_PAIR(COLOR_BLACK));

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
		game.render();

		// first color the player position if the cursor has moved from the player position
		if (targetCursorY != player->posY || targetCursorX != player->posX)
		{
			mvchgat(lastY, lastX, 1, A_NORMAL, WHITE_PAIR, nullptr);
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
			mvchgat(lineY, lineX, 1, A_STANDOUT, WHITE_PAIR, nullptr);
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
		if (game.map->is_in_fov(targetCursorX, targetCursorY))
		{
			auto actor = game.map->get_actor(targetCursorX, targetCursorY);
			// and actor is not an item
			if (actor != nullptr && actor->destructible != nullptr)
			{
				mvprintw(0, 0, actor->name.c_str());
				// print the monster's stats
				mvprintw(1, 0, "HP: %d/%d", actor->destructible->hp, actor->destructible->hpMax);
				mvprintw(2, 0, "AC: %d", actor->destructible->dr);
			}
		}

		// highlight the possible range of the explosion make it follow the cursor

		// get the center of the explosion
		int centerX = targetCursorX;
		int centerY = targetCursorY;

		// get the radius of the explosion
		int radius = maxRange;
		
		int sideLength = radius * 2 + 1;

		// calculate the chebyshev distance from the player to maxRange
		int chebyshevD = std::max(abs(centerX - (centerX - radius)), abs(centerY - (centerY - radius)));

		int height = sideLength;
		int width = sideLength;

		// Calculate the position of the aoe window
		int centerOfExplosionY = centerY - chebyshevD;
		int centerOfExplosionX = centerX - chebyshevD;


		// Move the window to the new position
		mvwin(aoe, centerOfExplosionY - 1, centerOfExplosionX - 1);
		wrefresh(aoe);
		
		box(aoe, 0, 0);

		wbkgd(aoe, COLOR_PAIR(COLOR_BLACK));
		if (aoe != nullptr)
		{
			wrefresh(aoe); // After continuosly pressing mouse to move the aoe window. Exception thrown at 0x62D79032 (pdcurses.dll) in C++RogueLike.exe: 0xC0000005: Access violation reading location 0x0800EFED. 
		}
		else
		{
			std::cout << "aoe is null" << std::endl;
			exit(-1);
		}

		////display the FOV in white
		// 
		//for (int tilePosX = 0; tilePosX < game.map->map_width; tilePosX++)
		//{
		//	for (int tilePosY = 0; tilePosY < game.map->map_height; tilePosY++)
		//	{
		//		if (game.map->is_in_fov(tilePosX, tilePosY))
		//		{
		//			mvchgat(tilePosY, tilePosX, 1, A_REVERSE, LIGHTNING_PAIR, NULL);
		//		}
		//	}
		//}

		// draw the AOE in white
		for (int tilePosX = targetCursorX - (chebyshevD - 1 - 1 + 2) ; tilePosX < (centerOfExplosionX + (width - 1 + 1)); tilePosX++)
		{
			for (int tilePosY = targetCursorY - (chebyshevD - 1 - 1 + 2); tilePosY < (centerOfExplosionY + (height - 1 + 1)); tilePosY++)
			{
				{
					mvchgat(tilePosY, tilePosX, 1, A_REVERSE, LIGHTNING_PAIR, nullptr);
				}
			}
		}

		///DEBUG
		//mvprintw(0, 0, "param X : %d", *x);
		//mvprintw(1, 0, "param Y : %d", *y);
		//mvprintw(2, 0, "targetCursorX : %d", targetCursorX);
		//mvprintw(3, 0, "targetCursorY : %d", targetCursorY);
		//mvprintw(4, 0, "lastX : %d", lastX);
		//mvprintw(5, 0, "lastY : %d", lastY);
		//mvprintw(6, 0, "lineX : %d", lineX);
		//mvprintw(7, 0, "lineY : %d", lineY);
		//mvprintw(8, 0, "centerX : %d", centerX);
		//mvprintw(9, 0, "centerY : %d", centerY);
		//mvprintw(10, 0, "radius : %d", radius);
		//mvprintw(11, 0, "sideLength : %d", sideLength);
		//mvprintw(12, 0, "chebyshevD : %d", chebyshevD);
		//mvprintw(13, 0, "height : %d", height);
		//mvprintw(14, 0, "width : %d ", width);
		//mvprintw(15, 0, " x : %d ", *x);
		//mvprintw(16, 0, " y : %d ", *y);
		//mvprintw(17, 0, "centerOfExplosionX : %d", centerOfExplosionX);
		//mvprintw(18, 0, "centerOfExplosionY : %d", centerOfExplosionY);

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
			if (game.map->is_in_fov(targetCursorX, targetCursorY))
			{
				auto actor = game.map->get_actor(targetCursorX, targetCursorY);
				// and actor is not an item
				if (actor != nullptr && actor->destructible != nullptr)
				{
					/*player->attacker->attack(player, actor);*/
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
	// Delete the window after the loop
	delwin(aoe);
	clear();

	return false;
}

bool Game::mouse_moved()
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

void Game::target()
{
	int targetCursorY = player->posY; // init position Y
	int targetCursorX = player->posX; // init position X

	const int lastY = targetCursorY;
	const int lastX = targetCursorX;

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
		game.render();

		//display the FOV in white
		
		for (int tilePosX = 0; tilePosX < game.map->map_width; tilePosX++)
		{
			for (int tilePosY = 0; tilePosY < game.map->map_height; tilePosY++)
			{
				if (game.map->is_in_fov(tilePosX, tilePosY))
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
		const int distance = player->get_distance(targetCursorX, targetCursorY);
		if (game.map->is_in_fov(targetCursorX, targetCursorY))
		{
			auto actor = game.map->get_actor(targetCursorX, targetCursorY);
			// and actor is not an item
			if (actor != nullptr && actor->destructible != nullptr )
			{
				mvprintw(0, 0, actor->name.c_str());
				// print the monster's stats
				mvprintw(1, 0, "HP: %d/%d", actor->destructible->hp, actor->destructible->hpMax);
				mvprintw(2, 0, "AC: %d", actor->destructible->dr);
				mvprintw(3, 0, "STR: %d", actor->attacker->dmg);
				// print the distance from the player to the target cursor
				mvprintw(0, 50, "Distance: %d", distance);
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
			if (game.map->is_in_fov(targetCursorX, targetCursorY))
			{
				auto actor = game.map->get_actor(targetCursorX, targetCursorY);
				// and actor is not an item
				if (actor != nullptr && actor->destructible != nullptr)
				{
					player->attacker->attack(*player, *actor);
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

void Game::load_all()
{
	if (TCODSystem::fileExists("game.sav"))
	{
		TCODZip zip;

		zip.loadFromFile("game.sav");

		// load the map
		int width = zip.getInt();
		int height = zip.getInt();
		game.map = std::make_unique<Map>(height, width);
		map->load(zip);

		// load the player
		/*player = std::make_shared<Actor>(0, 0, 0, "loaded player", EMPTY_PAIR, 0);*/
		/*player = std::make_shared<Player>(0, 0, 0, "loaded player", EMPTY_PAIR, 0);*/
		player->load(zip);
		actors.push_back(player);

		// load the stairs
		stairs = std::make_shared<Actor>(0, 0, 0, "loaded stairs", WHITE_PAIR, 0);
		stairs->load(zip);
		actors.push_back(stairs);
		/*actors.try_emplace(stairs->index, stairs);*/

		// then all other actors
		int nbActors = zip.getInt();
		while (nbActors > 0)
		{
			/*Actor* actor = new Actor(0, 0, 0, "loaded other actors", EMPTY_PAIR);*/
			std::shared_ptr<Actor> actor = std::make_shared<Actor>(0, 0, 0, "loaded other actors", EMPTY_PAIR, 0);
			actor->load(zip);
			actors.push_back(actor);
			/*actors.try_emplace(actor->index, actor);*/
			nbActors--;
		}

		// finally the message log
		gui->load(zip);
	}
	// TODO : Delete this line moved to switch case in game_menu
	else
	{
		game.init();
	}
}

void Game::save_all()
{
	std::clog << "Saving the game..." << std::endl;
	if (player != nullptr)
	{
		if (player->destructible != nullptr)
		{

			// handle the permadeath
			// delete the save file if the player is dead
			if (player->destructible->is_dead())
			{
				// if file exists delete it
				if (TCODSystem::fileExists("game.sav"))
				{
					TCODSystem::deleteFile("game.sav");
				}


				/*TCODSystem::deleteFile("game.sav");*/
			}
			// else save the map
			else
			{
				if (map != nullptr)
				{
					std::clog << "saving..." << std::endl;

					TCODZip zip;

					// save the map first
					zip.putInt(map->map_width);
					zip.putInt(map->map_height);
					map->save(zip);

					if (player != nullptr)
					{
						player->save(zip); // save the player
					}
					else
					{
						std::clog << "player is null in Game::save()" << std::endl;
					}

					if (stairs != nullptr)
					{
						stairs->save(zip); // save the stairs
					}
					else
					{
						std::clog << "player is null in Game::save()" << std::endl;
					}

					if(!actors.empty())
					{
						// then all the other actors
						int nbActors = gsl::narrow_cast<int>(actors.size()); // actors will never be larger than the maximum value of an int, then using gsl::narrow_cast<int> can be considered okay
						int nbActorsToSave = nbActors - 2; // -2 because player and stairs are already saved
						zip.putInt(nbActorsToSave);
					}
					else 
					{
						std::clog << "actors is empty in Game::save()" << std::endl;
					}

					for (const auto& actor : actors)
					{
						if (actor != nullptr)
						{
							if (actor != player && actor != stairs)
							{
								actor->save(zip);
							}
						}
						else
						{
							std::clog << "actor is null in Game::save()" << std::endl;
						}
					}

					if (gui != nullptr)
					{
						// save the message log
						gui->save(zip);
					}
					else
					{
						std::clog << "gui is null in Game::save()" << std::endl;
					}

					zip.saveToFile("game.sav");
				}
				else
				{
					std::clog << "map is null in Game::save()" << std::endl;
				}
			}
		}
		else
		{
			std::clog << "player->destructible is null in Game::save()" << std::endl;
			return;
		}
	}
	else
	{
		std::clog << "player is null in Game::save()" << std::endl;
		return;
	}
}

void Game::next_level()
{
	dungeonLevel++;
	gui->log_message(WHITE_PAIR, "You take a moment to rest, and recover your strength.");
	player->destructible->heal(player->destructible->hpMax / 2);
	gui->log_message(WHITE_PAIR, "After a rare moment of peace, you descend\ndeeper into the heart of the dungeon...");
	gui->log_message(WHITE_PAIR, "You are now on level %d", dungeonLevel);

	// clear the dungeon except the player and the stairs
	actors.clear();
	actors.push_back(player);
	actors.push_back(stairs);

	map = std::make_unique<Map>(MAP_HEIGHT, MAP_WIDTH);
	map->init(true);

	gameStatus = GameStatus::STARTUP;
}

// create the getActor function
std::shared_ptr<Actor> Game::get_actor(int x, int y) const
{
	//for (Actor* actor : actors)
	//{
	//	if (actor->posX == x && actor->posY == y)
	//	{
	//		return actor;
	//	}
	//}

	for (auto& actor : actors)
	{
		if (actor->posX == x && actor->posY == y)
		{
			return actor;
		}
	}

	return nullptr;
}

void Game::dispay_levelup(int xpLevel)
{
	RandomDice d;

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
		mvwprintw(stats, 2, 1, "Level: %d", xpLevel);
		mvwprintw(stats, 3, 1, "Experience: %d", player->destructible->xp);
		mvwprintw(stats, 4, 1, "Food: %d/%d", player->destructible->food, player->destructible->foodMax);
		mvwprintw(stats, 5, 1, "Need to sleep: %d", player->destructible->needToSleep);
		mvwprintw(stats, 6, 1, "[a] Attack: %d", player->attacker->dmg);
		mvwprintw(stats, 7, 1, "[d] Defense: %d", player->destructible->dr);
		mvwprintw(stats, 8, 1, "[h] Health: %d/%d", player->destructible->hp, player->destructible->hpMax);
		
		wrefresh(stats);

		const int key = getch();
		switch (key)
		{

		case 'a':
		{
			const int roll = d.d4();
			player->attacker->dmg += d.d4();
			break;
		}

		case 'd':
		{
			player->destructible->dr += 1;
			break;
		}

		case 'h':
		{
			const int roll = d.d8();
			player->destructible->hpMax += d.d8();
			break;
		}

		default:
			continue;
		}

		break;
	}

	if (stats)
	{
		delwin(stats);
	}

	clear();
}

// display character sheet
void Game::display_character_sheet() noexcept
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
	auto run{ true };
	while (run == true)
	{
		// display the player stats
		// using a dnd character sheet
		// based on https://wiki.roll20.net/ADnD_2nd_Edition_Character_sheet

		// display the player name
		mvwprintw(character_sheet, 1, 1, "Name: %s", player->name.c_str());
		// display the player class
		mvwprintw(character_sheet, 2, 1, "Class: %s", player->playerClass.c_str());
		// display the class kit
		mvwprintw(character_sheet, 3, 1, "Kit: ");
		// display the player level
		mvwprintw(character_sheet, 4, 1, "Level: ");
		// display the player experience
		mvwprintw(character_sheet, 5, 1, "Experience: %d", player->destructible->xp);
		// display the player alignment
		mvwprintw(character_sheet, 6, 1, "Alignment: ");
		// add character details on the right side
		// display the player race
		mvwprintw(character_sheet, 1, 60, "Race: ");
		// display gender
		mvwprintw(character_sheet, 2, 60, "Gender: %s", player->gender.c_str());
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

		const int key = getch();
		// if any key was pressed then exit the loop
		if (key != ERR)
		{
			run = false;
		}
	}
	delwin(character_sheet);
	clear();
}

// make a random number function to use in the game
// TODO : does this belong in the engine class?
int Game::random_number(int min, int max)
{
	static std::default_random_engine randomEngine(gsl::narrow_cast<unsigned int>(time(nullptr)));
	std::uniform_int_distribution<int> range(min, max);
	
	return range(randomEngine);
}

// displays the actors names
// TODO: this is a test function
void Game::wizard_eye() noexcept
{
	for (const auto& actor : game.actors)
	{
		// print the actor's name
		mvprintw(actor->posY, actor->posX, actor->name.c_str());
	}
}

// end of file: Game.cpp
