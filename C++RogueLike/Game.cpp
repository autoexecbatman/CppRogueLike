// file: Game.cpp
#include <iostream>
#include <fstream>
#include <curses.h>
#include <algorithm> // for std::remove in sendToBack(Actor*)
#include <random>
#include <climits>
#include <cassert>
#include <gsl/util>
#include <format>

#include "Game.h"
#include "Actor/Actor.h"
#include "Actor/Attacker.h"
#include "Actor/Pickable.h"
#include "Actor/Container.h"
#include "Ai/Ai.h"
#include "Ai/AiPlayer.h"
#include "Map/Map.h"
#include "Gui/Gui.h"
#include "Gui/Window.h"
#include "Colors/Colors.h"
#include "Random/RandomDice.h"
#include "ActorTypes/Player.h"
#include "Menu/Menu.h"
#include "Menu/MenuGender.h"
#include "Menu/MenuRace.h"
#include "Menu/MenuClass.h"
#include "Menu/MenuName.h"
#include "dnd_tables/CalculatedTHAC0s.h"

//==INIT==
// When the Game is created, 
// We don't know yet if we have to generate a new map or load a previously saved one.
// Will be called in load_all() when loading and menu() for new game
void Game::init()
{
	//==INIT==
	game.weapons = loadWeapons();
	game.strengthAttributes = loadStrengthAttributes();

	//==STAIRS==
	if (!game.stairs) { err("game.stairs is nullptr"); }
	game.stairs->blocks = false; // stairs are not blocking
	game.stairs->fovOnly = false; // stairs are not fovOnly
	game.actors.push_back(std::move(stairs_unique));
	game.stairs = game.actors.back().get();

	//==MAP==
	game.map->init(true); // set the checker function

	//==GameStatus==
	// we set gameStatus to STARTUP because we want to compute the fov only once
	gameStatus = GameStatus::STARTUP;
	game.log("GameStatus::STARTUP");

	//==LOG==
	game.log("game.init() was called!");
}

//==CREATE PLAYER==
// This function sets the arguments to the player pointer
void Game::create_player()
{
	//==PLAYER==
	RandomDice d; // we create a RandomDice object to roll the dice
	int playerHp = 20 + d.d10(); // we roll the dice to get the player's hp
	int playerDamage = 2 + d.d8(); // we roll the dice to get the player's damage
	int playerMinDmg = 2; // the player's minimum damage
	int playerMaxDmg = 8; // the player's maximum damage
	int playerDr = 1; // the player's damage reduction
	int playerXp = 0; // the player's experience points
	int playerTHAC0 = game.player->destructible->thaco; // the player's THAC0
	int playerAC = 0; // the player's armor class
	const int playerLevel = game.player->playerLevel; // the player's level

	// update the player pointer
	game.player_unique = std::make_unique<Player>(0, 0, playerHp, playerDr, "your cadaver", playerXp, playerTHAC0, playerAC, playerDamage, playerMinDmg, playerMaxDmg);
	
	// add the player to the actors vector for rendering
	game.actors.push_back(std::move(game.player_unique));
	game.player = dynamic_cast<Player*>(game.actors.back().get());

	// TODO :
	// 1. Set the playerHp to rolls based on class and attribute modifiers.
	// 2. Set the playerDamage to rolls based on class and attribute modifiers and weapon.
	// 3. Set the playerAC to be based on base AC and attribute modifiers and armor.

}

//==UPDATE==
// the update function to update the game logic
// and stores events
void Game::update()
{
	// permadeath is enabled !
	// if the player is dead, we end the game
	if (game.player->destructible->is_dead())
	{
		game.log("Player is dead!"); // log the event

		game.appendMessagePart(COLOR_RED, "You died! Press any key...");
		game.finalizeMessage();

		// set the game loop to stop
		run = false;
	}
	else
	{
		// still alive, we update the game logic
		// to compute the FOV only once for performance
		// gameStatus should be set to STARTUP
		if (Game::gameStatus == GameStatus::STARTUP)
		{
			game.log("...Computing FOV...");
			game.map->compute_fov();

			// adjust the attributes based on players race
			game.player->racial_ability_adjustments();
			game.player->calculate_thaco();
		}

		// we set the gameStatus to IDLE
		// IDLE is the default state
		gameStatus = GameStatus::IDLE;
		game.log("GameStatus::IDLE");

		// in the update procedure if the player has moved
		// we set the gameStatus to NEW_TURN
		// or IDLE if the player has not moved
		game.log("Updating player...");
		game.player->update();
		game.log("Player updated!");

		// if the player moved we update the world
		game.log("Updating actors...");
		if (Game::gameStatus == GameStatus::NEW_TURN)
		{
			for (const auto& actor : actors)
			{
				if (actor->is_visible() && actor != nullptr && actor.get() != player)
				{
					game.log("Actor: " + actor->name + " is in FOV");
					actor->update();
				}
			}
		}
		game.log("Actors updated!");
	}
}

//==RENDERING==
// the engine render function implementation
// draws the entities on the map
void Game::render()
{
	// we try to render the map first
	try { map->render(); }
	catch (const std::exception& e)
	{
		game.log(e.what());
		exit(-1);
	}

	game.log("Actors are trying to be drawn...");
	for (const auto& actor : actors)
	{
		if (actor && actor.get() != player)
		{
			if (actor->is_visible())
			{
				game.log("Actor: " + actor->name + " is in FOV");
				actor->render();
				std::clog << "Actor: " << actor->name << " is drawn" << std::endl;
			}
		}
	}
	game.log("Actors are drawn");

	game.log("Player is trying to be drawn...");
	player->render();
	game.log("Player is drawn");

	game.log("RENDER FUNCTION OUT");
	// heavy on the logging here because there is alot happening in this functions
}

void Game::send_to_back(Actor& actor)
{
	auto actorIsInVector = [&actor](const auto& a) noexcept { return a.get() == &actor; }; // lambda to check if the actor is in the vector
	auto it = std::find_if(actors.begin(), actors.end(), actorIsInVector); // get the iterator of the actor
	const auto distance = std::distance(actors.begin(), it); // get the distance from the begining of the vector to the actor
	for (auto i = distance; i > 0; i--)
	{
		// swap actor with the previous actor
		std::swap(gsl::at(actors, i - 1), gsl::at(actors, i));
	}
}

Actor* Game::get_closest_monster(int fromPosX, int fromPosY, double inRange) const noexcept
{
	Actor* closestMonster = nullptr;
	int bestDistance = INT_MAX;

	for (const auto& actor : actors)
	{
		if (actor.get() != game.player && !actor->destructible->is_dead())
		{
			const int distance = actor->get_distance(fromPosX, fromPosY);
			if (distance < bestDistance && (distance <= inRange || inRange == 0.0f))
			{
				bestDistance = distance;
				closestMonster = actor.get();
			}
		}
	}

	return closestMonster;
}

bool Game::pick_tile(int* x, int* y, int maxRange)
{
	// the target cursor is initialized at the player's position
	int targetCursorY = player->posY; // init position Y
	int targetCursorX = player->posX; // init position X

	// initialize the line position
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

		// first color the player position if the cursor has moved from the player position
		if (targetCursorY != player->posY || targetCursorX != player->posX)
		{
			mvchgat(player->posY, player->posX, 1, A_NORMAL, WHITE_PAIR, nullptr);
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

		attron(COLOR_PAIR(HPBARMISSING_PAIR));
		mvaddch(targetCursorY, targetCursorX, 'X');
		attroff(COLOR_PAIR(HPBARMISSING_PAIR));

		// if the cursor is on a monster then display the monster's name
		if (game.map->is_in_fov(targetCursorX, targetCursorY))
		{
			const auto& actor = game.map->get_actor(targetCursorX, targetCursorY);
			// and actor is not an item
			if (actor != nullptr)
			{
				mvprintw(0, 0, actor->name.c_str());
				// print the monster's stats
				mvprintw(1, 0, "HP: %d/%d", actor->destructible->hp, actor->destructible->hpMax);
				mvprintw(2, 0, "AC: %d", actor->destructible->dr);
			}
		}
		
		// highlight the possible range of the explosion make it follow the cursor

		// get the center of the explosion
		const int centerX = targetCursorX;
		const int centerY = targetCursorY;

		// get the radius of the explosion
		const int radius = maxRange;
		
		const int sideLength = radius * 2 + 1;

		// calculate the chebyshev distance from the player to maxRange
		const int chebyshevD = std::max(abs(centerX - (centerX - radius)), abs(centerY - (centerY - radius)));

		const int height = sideLength;
		const int width = sideLength;

		// Calculate the position of the aoe window
		const int centerOfExplosionY = centerY - chebyshevD;
		const int centerOfExplosionX = centerX - chebyshevD;

		// draw the AOE in white
		for (int tilePosX = targetCursorX - chebyshevD; tilePosX < centerOfExplosionX + width; tilePosX++)
		{
			for (int tilePosY = targetCursorY - chebyshevD; tilePosY < centerOfExplosionY + height; tilePosY++)
			{
				{
					mvchgat(tilePosY, tilePosX, 1, A_REVERSE, LIGHTNING_PAIR, nullptr);
				}
			}
		}

		//// DEBUG
		//mvprintw(0, 0, "param X : %d", *x);
		//mvprintw(1, 0, "param Y : %d", *y);
		//mvprintw(2, 0, "targetCursorX : %d", targetCursorX);
		//mvprintw(3, 0, "targetCursorY : %d", targetCursorY);
		//mvprintw(4, 0, "lineX : %d", lineX);
		//mvprintw(5, 0, "lineY : %d", lineY);
		//mvprintw(6, 0, "centerX : %d", centerX);
		//mvprintw(7, 0, "centerY : %d", centerY);
		//mvprintw(8, 0, "radius : %d", radius);
		//mvprintw(9, 0, "sideLength : %d", sideLength);
		//mvprintw(10, 0, "chebyshevD : %d", chebyshevD);
		//mvprintw(11, 0, "height : %d", height);
		//mvprintw(12, 0, "width : %d ", width);
		//mvprintw(13, 0, "centerOfExplosionX : %d", centerOfExplosionX);
		//mvprintw(14, 0, "centerOfExplosionY : %d", centerOfExplosionY);

		refresh();

		// get the key press
		const int key = getch();
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
				const auto& actor = game.map->get_actor(targetCursorX, targetCursorY);
				// and actor is not an item
				if (actor != nullptr)
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

	return false;
}

void Game::run_menus() {
	Menu mainMenu;
	MenuGender menuGender;
	MenuRace menuRace;
	// If you have more menus, define them here...

	while (game.run) {
		// Main menu
		mainMenu.menu_set_run_true();
		mainMenu.menu();
		if (!mainMenu.run) break; // If main menu is quit, break out of loop

		// Gender menu
		menuGender.menu_gender_set_run_true();
		menuGender.menu_gender();
		if (!menuGender.run) continue; // If back is pressed in gender menu, continue to main menu

		// Race menu
		menuRace.menu_race_set_run_true();
		menuRace.menu_race();
		if (!menuRace.run) continue; // If back is pressed in race menu, continue to gender menu

		// If you have more menus, add them here following the same pattern...

		// Init the game after all menus
		game.init();
	}
}

bool Game::mouse_moved() noexcept
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
			const auto& actor = game.map->get_actor(targetCursorX, targetCursorY);
			// and actor is not an item
			if (actor != nullptr)
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
		const int key = getch();
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
				const auto& actor = game.map->get_actor(targetCursorX, targetCursorY);
				
				// and actor is not an item
				if (actor != nullptr)
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
		} // end of switch (key)

	} // end of while (run == true)
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
		map->map_height = height;
		map->map_width = width;

		map->load(zip);

		// load the player
		/*player = std::make_shared<Actor>(0, 0, 0, "loaded player", EMPTY_PAIR, 0);*/
		/*player = std::make_shared<Player>(0, 0, 0, "loaded player", EMPTY_PAIR, 0);*/
		player->load(zip);
		/*actors.push_back(std::move(player));*/

		// load the stairs
		stairs_unique = std::make_unique<Actor>(0, 0, 0, "loaded stairs", WHITE_PAIR, 0);
		stairs_unique->load(zip);
		actors.push_back(std::move(stairs_unique));
		/*actors.try_emplace(stairs->index, stairs);*/

		// then all other actors
		int nbActors = zip.getInt();
		while (nbActors > 0)
		{
			/*Actor* actor = new Actor(0, 0, 0, "loaded other actors", EMPTY_PAIR);*/
			std::unique_ptr<Actor> actor = std::make_unique<Actor>(0, 0, 0, "loaded other actors", EMPTY_PAIR, 0);
			actor->load(zip);
			actors.push_back(std::move(actor));
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
	try
	{
		if (!shouldSave) { game.log("You quit without saving."); return; } // don't save if quit from the menu without playing

		if (player->destructible->is_dead() && TCODSystem::fileExists("game.sav"))
		{
			// handle the permadeath
			// delete the save file if the player is dead or the save file is corrupted
			TCODSystem::deleteFile("game.sav");
			return;
		}
		else // else save the game
		{
			TCODZip zip; // create a zip object

			// save the map
			zip.putInt(map->map_width);
			zip.putInt(map->map_height);
			map->save(zip);

			// save the player
			player->save(zip);
			
			// save the stairs
			if (stairs) try { stairs->save(zip); }
			catch (const std::exception& e) { game.log(e.what()); }

			// save the other actors
			if (!actors.empty())
			{
				const int nbActors = gsl::narrow_cast<int>(actors.size()); // actors will never be larger than the maximum value of an int, then using gsl::narrow_cast<int> can be considered okay
				const int nbActorsToSave = nbActors - 2; // -2 because player and stairs are already saved
				zip.putInt(nbActorsToSave);

				for (const auto& actor : actors)
				{
					if (actor != nullptr && actor.get() != player && actor.get() != stairs)
					{
						actor->save(zip);
					}
				}
			}

			// save the message log
			if (gui) try { gui->save(zip); }
			catch (const std::exception& e) { game.log(e.what()); }

			zip.saveToFile("game.sav");
			game.log("Game saved successfully.");
		}
	}
	catch (const std::exception& e)
	{
		game.log("Error occurred while saving: " + std::string(e.what()));
		throw;
	}
}

void Game::next_level()
{
	// increment the dungeon level
	dungeonLevel++;

	// present a message to the player
	game.message(WHITE_PAIR, "You take a moment to rest, and recover your strength.",true);

	// heal the player
	player->destructible->heal(player->destructible->hpMax / 2);

	// present a message to the player

	game.message(WHITE_PAIR, "deeper into the heart of the dungeon...", true);
	game.message(WHITE_PAIR, "After a rare moment of peace, you descend",true);
	game.message(WHITE_PAIR, std::format("You are now on level {}", dungeonLevel), true);

	auto actorIsPlayer = [this](const auto& actor) noexcept { return actor.get() == player; };
	auto actorIsStairs = [this](const auto& actor) noexcept { return actor.get() == stairs; };

	// find the player in the actors container
	auto it = std::find_if(actors.begin(), actors.end(), actorIsPlayer);
	// find the stairs in the actors container
	auto itStairs = std::find_if(actors.begin(), actors.end(), actorIsStairs);

	// move the player to a temporary variable
	auto tempPlayer = std::move(*it);
	// move the stairs to a temporary variable
	auto tempStairs = std::move(*itStairs);

	// clear the actors container except the player and the stairs
	actors.clear();

	// add the player and the stairs to the actors container
	actors.push_back(std::move(tempPlayer));
	actors.push_back(std::move(tempStairs));
	player = dynamic_cast<Player*>(actors.front().get());
	stairs = actors.back().get();

	// generate a new map
	map->map_height = MAP_HEIGHT;
	map->map_width = MAP_WIDTH;
	map->init(true);

	// set the game status to STARTUP because we need to recompute the FOV 
	gameStatus = GameStatus::STARTUP;
}

Actor* Game::get_actor(int x, int y) const noexcept
{
	for (const auto& actor : actors)
	{
		if (actor->posX == x && actor->posY == y)
		{
			return actor.get();
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
		1, // y
		1 // x
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
		mvwprintw(character_sheet, 4, 1, "Level: %d", player->playerLevel);
		// display the player experience
		mvwprintw(character_sheet, 5, 1, "Experience: %d", player->destructible->xp);
		// display the player alignment
		mvwprintw(character_sheet, 6, 1, "Alignment: ");
		// add character details on the right side
		// display the player race
		mvwprintw(character_sheet, 1, 60, "Race: %s", player->playerRace.c_str());
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

// displays the actors as names
void Game::wizard_eye() noexcept
{
	for (const auto& actor : game.actors)
	{
		// print the actor's name
		mvprintw(actor->posY, actor->posX, actor->name.c_str());
	}
}

void Game::log(const std::string& message)
{
	if (debugMode)
	{
		std::clog << message << std::endl;
		std::cout << message << std::endl;
	}
}

void Game::message(int color, const std::string& text, bool isComplete = false)
{
	// store message in game
	messageToDisplay = text;
	messageColor = color;

	// Always append the message part to attackMessageParts
	attackMessageParts.push_back({ color, text });

	// If isComplete flag is set, consider the message to be finished
	if (isComplete) {
		// Add the entire composed message parts to attackMessagesWhole
		attackMessagesWhole.push_back(attackMessageParts);

		// Clear attackMessageParts for the next message
		attackMessageParts.clear();
	}

	game.log("Stored message: '" + messageToDisplay + "'");
	game.log("Stored message color: " + std::to_string(messageColor));

}

void Game::appendMessagePart(int color, const std::string& text)
{
	attackMessageParts.push_back({ color, text });
}

void Game::finalizeMessage()
{
	if (!attackMessageParts.empty()) {
		attackMessagesWhole.push_back(attackMessageParts);
		attackMessageParts.clear();
	}
}

void Game::display_debug_messages() noexcept
{
	clear();
	int total_lines = 0; // To hold the total number of lines in the file
	std::ifstream logFile("clog.txt");
	std::string line;

	// First, count the number of lines in the file
	while (getline(logFile, line))
	{
		total_lines++;
	}
	logFile.close();

	// Create a pad large enough to hold all the text
	WINDOW* log_pad = newpad(total_lines + 1, COLS - 2);
	int y = 0;

	// Open the file again to actually display the text
	logFile.open("clog.txt");
	if (logFile.is_open())
	{
		while (getline(logFile, line))
		{
			mvwprintw(log_pad, y++, 1, "%s", line.c_str());
		}
		logFile.close();
	}

	// Initial display position
	int pad_pos = 0;
	prefresh(log_pad, pad_pos, 0, 1, 1, LINES - 2, COLS - 2);

	// Scroll interaction
	int ch;
	do
	{
		ch = getch();
		switch (ch)
		{
		case KEY_DOWN:
			if (pad_pos + LINES - 2 < total_lines)
			{
				pad_pos++;
			}
			break;
		case KEY_UP:
			if (pad_pos > 0)
			{
				pad_pos--;
			}
			break;
		case KEY_NPAGE:  // Handle Page Down
			if (pad_pos + LINES - 2 < total_lines)
			{
				pad_pos += (LINES - 2);  // Move down a page
				if (pad_pos + LINES - 2 > total_lines)
				{  // Don't go past the end
					pad_pos = total_lines - LINES + 2;
				}
			}
			break;
		case KEY_PPAGE:  // Handle Page Up
			if (pad_pos > 0)
			{
				pad_pos -= (LINES - 2);  // Move up a page
				if (pad_pos < 0)
				{
					pad_pos = 0;  // Don't go past the beginning
				}
			}
			break;
		case KEY_HOME:  // Jump to the top of the log
			pad_pos = 0;
			break;
		case KEY_END:  // Jump to the bottom of the log
			if (total_lines > LINES - 2)
			{
				pad_pos = total_lines - LINES + 2;
			}
			break;
		}
		prefresh(log_pad, pad_pos, 0, 1, 1, LINES - 2, COLS - 2);
	} while (ch != 'q');  // Assuming 'q' or Escape closes the log

	delwin(log_pad);  // Delete the pad after use
}

// A possible function in Game.cpp

void Game::transferMessagesToGui()
{
	for (const auto& message : attackMessagesWhole)
	{
		gui->addDisplayMessage(message);
	}
	attackMessagesWhole.clear();
}

// end of file: Game.cpp
