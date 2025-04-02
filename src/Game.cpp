// file: Game.cpp
#include <iostream>
#include <fstream>
#include <curses.h>
#include <algorithm> // for std::remove in sendToBack(Actor*)
#include <ranges>
#include <random>
#include <climits>
#include <cassert>
#include <gsl/util>
#include <format>
#include <span>

#include "Game.h"
#include "Items.h"
#include "Actor/Actor.h"
#include "Actor/Attacker.h"
#include "Actor/Pickable.h"
#include "Actor/Container.h"
#include "ActorTypes/Player.h"
#include "ActorTypes/Monsters.h"
#include "Ai/Ai.h"
#include "Ai/AiPlayer.h"
#include "Map/Map.h"
#include "Gui/Gui.h"
#include "Gui/Window.h"
#include "Colors/Colors.h"
#include "Random/RandomDice.h"
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
	game.stairs = std::make_unique<Stairs>(Vector2D{ 0,0 });

	//==MAP==
	game.map->init(true); // with actors true

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
	const int playerHp = 20 + d.d10(); // we roll the dice to get the player's hp
	const int playerDr = 1; // the player's damage reduction
	const int playerXp = 0; // the player's experience points
	const int playerTHAC0 = game.player->destructible->thaco; // the player's THAC0
	const int playerAC = 0; // the player's armor class

	// update the player pointer
	game.player = std::make_unique<Player>(
		Vector2D{ 0, 0 },
		playerHp,
		playerDr,
		"your cadaver",
		playerXp,
		playerTHAC0,
		playerAC
	);

	// TODO :
	// 1. Set the playerHp to rolls based on class and attribute modifiers.
	// 2. Set the playerDamage to rolls based on class and attribute modifiers and weapon.
	// 3. Set the playerAC to be based on base AC and attribute modifiers and armor.
}

void Game::update_creatures(std::span<std::unique_ptr<Creature>> creatures)
{
	for (const auto& creature : creatures)
	{
		if (creature)
		{
			creature->update();
		}
	}
}

void Game::render_creatures(std::span<std::unique_ptr<Creature>> creatures)
{
	for (const auto& creature : creatures)
	{
		if (creature)
		{
			creature->render();
		}
	}
}

void Game::spawn_creatures() const
{
	// add a new monster every 5 turns
	if (game.time % 5 == 0)
	{
		// if there are less than 6 monsters on the map
		if (creatures.size() < MAX_MONSTERS)
		{
			// game.rooms must be populated
			if (game.rooms.empty())
			{
				throw std::runtime_error("game.rooms is empty!");
			}
			// roll a random index as the size of the rooms vector
			int index = game.d.roll(0, static_cast<int>(game.rooms.size()) - 1);
			// make the index even
			index = index % 2 == 0 ? index : index - 1;
			// get the room begin and end
			const Vector2D roomBegin = game.rooms.at(index);
			const Vector2D roomEnd = game.rooms.at(index + 1);
			// get a random position in the room
			Vector2D pos = Vector2D{ game.d.roll(roomBegin.y, roomEnd.y), game.d.roll(roomBegin.x, roomEnd.x) };
			// if pos is at wall roll again
			while (!game.map->can_walk(pos))
			{
				pos.x = game.d.roll(roomBegin.x, roomEnd.x);
				pos.y = game.d.roll(roomBegin.y, roomEnd.y);
			}
			// add a monster to the map
			game.map->add_monster(pos);
		}
	}
}

void Game::render_items(std::span<std::unique_ptr<Item>> items)
{
	for (const auto& item : items)
	{
		if (item)
		{
			item->render();
		}
	}
}

void Game::handle_menus()
{
	if (!game.menus.empty())
	{
		game.menus.back()->menu();
		// if back is pressed, pop the menu
		if (game.menus.back()->back)
		{
			game.menus.pop_back();
			game.menus.back()->menu_set_run_true();
		}

		if (!game.menus.back()->run)
		{
			game.menus.pop_back();
		}

		game.shouldInput = false;
	}
}

void Game::handle_gameloop(Gui& gui, int loopNum)
{
	if (!game.gameInit)
	{
		game.init();
		game.gameInit = true;
	}
	//==DEBUG==
	game.log("//====================LOOP====================//");
	game.log("Loop number: " + std::to_string(loopNum) + "\n");

	//==INIT_GUI==
	if (!gui.guiInit)
	{
		gui.gui_init();
		gui.guiInit = true;
	}

	//==INPUT==
	game.keyPress = ERR; // reset the keyPress so it won't get stuck in a loop
	if (game.shouldInput)
	{
		game.key_store();
		game.key_listen();
	}
	game.shouldInput = true; // reset shouldInput to reset the flag

	//==UPDATE==
	game.log("Running update...");
	game.update(); // update map and actors positions
	gui.gui_update(); // update the gui
	game.log("Update OK.");
	// **NEW**: If a menu was added, skip the rest of this loop
	if (!game.menus.empty())
	{
		game.windowState = Game::WindowState::MENU;
		return;
	}

	//==DRAW==
	game.log("Running render...");
	clear();
	game.render(); // render map and actors to the screen
	gui.gui_render(); // render the gui
	refresh();
	game.log("Render OK.");
}

void Game::update()
{
	game.map->update(); // sets tiles to explored
	game.player->update(); // if moved set to NEW_TURN else IDLE

	if (gameStatus == GameStatus::STARTUP)
	{
		game.map->compute_fov();
		// adjust the attributes based on players race
		game.player->racial_ability_adjustments();
		game.player->calculate_thaco();
		gameStatus = GameStatus::NEW_TURN;
	}

	if (gameStatus == GameStatus::NEW_TURN)
	{
		game.update_creatures(creatures);
		game.spawn_creatures();

		// Increase hunger every turn
		hunger_system.increase_hunger(1);

		// Apply hunger effects
		hunger_system.apply_hunger_effects();

		game.time++;
		if (gameStatus != GameStatus::DEFEAT) // for killing the player
		{
			gameStatus = GameStatus::IDLE; // reset back to IDLE to prevent getting stuck in NEW_TURN
		}
	}

	if (gameStatus == GameStatus::DEFEAT)
	{
		game.log("Player is dead!");
		game.appendMessagePart(COLOR_RED, "You died! Press any key...");
		game.finalizeMessage();
		run = false;
	}
}

void Game::render()
{
	map->render();
	game.stairs->render();
	render_items(container->inv);
	render_creatures(creatures);
	player->render();
}

Creature* Game::get_closest_monster(Vector2D fromPosition, double inRange) const noexcept
{
	Creature* closestMonster = nullptr;
	int bestDistance = INT_MAX;

	for (const auto& actor : creatures)
	{
		if (!actor->destructible->is_dead())
		{
			const int distance = actor->get_tile_distance(fromPosition);
			if (distance < bestDistance && (distance <= inRange || inRange == 0.0f))
			{
				bestDistance = distance;
				closestMonster = actor.get();
			}
		}
	}

	return closestMonster;
}

bool Game::pick_tile(Vector2D* position, int maxRange)
{
	// the target cursor is initialized at the player's position
	Vector2D targetCursor = player->position;
	bool run = true;
	while (run)
	{
		clear();

		// make the line follow the mouse position
		// if mouse move
		if (mouse_moved())
		{
			targetCursor = get_mouse_position();
		}
		game.render();

		// first color the player position if the cursor has moved from the player position
		if (targetCursor != player->position)
		{
			mvchgat(player->position.y, player->position.x, 1, A_NORMAL, WHITE_PAIR, nullptr);
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
		// initialize the line position
		Vector2D line{ 0,0 };
		TCODLine::init(player->position.x, player->position.y, targetCursor.x, targetCursor.y);
		while (!TCODLine::step(&line.x, &line.y))
		{
			mvchgat(line.y, line.x, 1, A_STANDOUT, WHITE_PAIR, nullptr);
		}

		attron(COLOR_PAIR(HPBARMISSING_PAIR));
		mvaddch(targetCursor.y, targetCursor.x, 'X');
		attroff(COLOR_PAIR(HPBARMISSING_PAIR));

		// if the cursor is on a monster then display the monster's name
		if (game.map->is_in_fov(targetCursor))
		{
			const auto& actor = game.map->get_actor(targetCursor);
			// and actor is not an item
			if (actor != nullptr)
			{
				mvprintw(0, 0, actor->actorData.name.c_str());
				// print the monster's stats
				mvprintw(1, 0, "HP: %d/%d", actor->destructible->hp, actor->destructible->hpMax);
				mvprintw(2, 0, "AC: %d", actor->destructible->dr);
			}
		}
		
		// highlight the possible range of the explosion make it follow the cursor

		// get the center of the explosion
		Vector2D center = targetCursor;

		// get the radius of the explosion
		const int radius = maxRange;
		
		const int sideLength = radius * 2 + 1;

		// calculate the chebyshev distance from the player to maxRange
		const int chebyshevD = std::max(abs(center.x - (center.x - radius)), abs(center.y - (center.y - radius)));

		const int height = sideLength;
		const int width = sideLength;

		// Calculate the position of the aoe window
		Vector2D centerOfExplosion = center - Vector2D{ chebyshevD, chebyshevD };

		// draw the AOE in white
		for (int tilePosX = center.x - chebyshevD; tilePosX < centerOfExplosion.x + width; tilePosX++)
		{
			for (int tilePosY = center.y - chebyshevD; tilePosY < centerOfExplosion.y + height; tilePosY++)
			{
				{
					mvchgat(tilePosY, tilePosX, 1, A_REVERSE, LIGHTNING_PAIR, nullptr);
				}
			}
		}
		refresh();

		// get the key press
		const int key = getch();
		switch (key)
		{
		case KEY_UP:
			// move the selection cursor up
			targetCursor.y--;
			break;

		case KEY_DOWN:
			// move the selection cursor down
			targetCursor.y++;
			break;

		case KEY_LEFT:
			// move the selection cursor left
			targetCursor.x--;
			break;

		case KEY_RIGHT:
			// move the selection cursor right
			targetCursor.x++;
			break;

		case 'f':
			// if the player presses the 'f' key
			// then the target selection is confirmed
			// and the target coordinates are returned
			
			// first display a message
			game.message(WHITE_PAIR, "Target confirmed", true);
			// then return the coordinates
			*position = targetCursor;

			return true;
			break;

		case 10:
			// if the key enter is pressed then select the target
			// and return the target position
			game.message(WHITE_PAIR, "Attack confirmed", true);
			// if the target is a monster then attack it
		{
			if (game.map->is_in_fov(targetCursor))
			{
				const auto& actor = game.map->get_actor(targetCursor);
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
			game.message(WHITE_PAIR, "Target selection canceled", true);
			// if the key escape is pressed then cancel the target selection
			run = false;
			break;

		default:break;
		}

	}

	return false;
}

void Game::run_menus() {
	//Menu mainMenu;
	//MenuGender menuGender;
	//MenuRace menuRace;
	//// If you have more menus, define them here...

	//while (game.run)
	//{
	//	// Main menu
	//	mainMenu.menu_set_run_true();
	//	mainMenu.menu();
	//	if (!mainMenu.run) break; // If main menu is quit, break out of loop

	//	// Gender menu
	//	menuGender.menu_gender_set_run_true();
	//	/*menuGender.menu_gender();*/
	//	if (!menuGender.run) continue; // If back is pressed in gender menu, continue to main menu

	//	// Race menu
	//	menuRace.menu_race_set_run_true();
	//	menuRace.menu_race();
	//	if (!menuRace.run) continue; // If back is pressed in race menu, continue to gender menu

	//	// If you have more menus, add them here following the same pattern...

	//	// Init the game after all menus
	//	game.init();
	//}
}

// check if the mouse has moved
bool Game::mouse_moved() noexcept
{
	auto oldMousePos{ get_mouse_position_old() };
	auto currentMousePos{ get_mouse_position() };
	return currentMousePos != oldMousePos;
}

Vector2D Game::get_mouse_position() noexcept
{
	request_mouse_pos();
	Vector2D mousePos{ 0,0 };
	mousePos.x = Mouse_status.x;
	mousePos.y = Mouse_status.y;
	return mousePos;
}

Vector2D Game::get_mouse_position_old() noexcept
{
	Vector2D oldMousePos{ 0,0 };
	oldMousePos.x = Mouse_status.x;
	oldMousePos.y = Mouse_status.y;
	return oldMousePos;
}

void Game::target()
{
	if (!game.player->has_state(ActorState::IS_RANGED))
	{
		game.message(WHITE_PAIR, "You are not ranged!", true);
		return;
	}

	Vector2D targetCursor = player->position;
	const Vector2D lastPosition = targetCursor;
	bool run = true;
	while (run)
	{
		clear();

		// make the line follow the mouse position
		// if mouse move
		if (mouse_moved())
		{
			targetCursor = get_mouse_position();
		}
		game.render();

		// display the FOV in white in row major order
		Vector2D pos{ 0,0 };
		for (; pos.y < MAP_HEIGHT; pos.y++)
		{
			for (; pos.x < MAP_WIDTH; pos.x++)
			{
				if (game.map->is_in_fov(pos))
				{
					mvchgat(pos.y, pos.x, 1, A_REVERSE, LIGHTNING_PAIR, NULL);
				}
			}
		}

		// first color the player position if the cursor has moved from the player position
		if (targetCursor != player->position)
		{
			mvchgat(lastPosition.y, lastPosition.x, 1, A_NORMAL, WHITE_PAIR, NULL);
		}

		//// draw a line using TCODLine class
		///*
		//@CppEx
		//// Going from point 5,8 to point 13,4
		//int x = 5, y = 8;
		//TCODLine::init(x, y, 13, 4);
		//do {
		//	// update cell x,y
		//} while (!TCODLine::step(&x, &y));
		//*/
		//Vector2D line{ 0,0 };
		//TCODLine::init(player->position.x, player->position.y, targetCursor.x, targetCursor.y);
		//while (!TCODLine::step(&line.x, &line.y))
		//{
		//	mvchgat(line.y, line.x, 1, A_STANDOUT, WHITE_PAIR, NULL);
		//}

		// draw the line using TCODPath
		// first create a path from the player to the target cursor
		map->tcodPath->compute(player->position.x, player->position.y, targetCursor.x, targetCursor.y);
		// then iterate over the path
		int x, y;
		while (map->tcodPath->walk(&x, &y, true))
		{
			mvchgat(y, x, 1, A_REVERSE, WHITE_PAIR, NULL);
		}
		// draw the target cursor
		attron(COLOR_PAIR(HPBARMISSING_PAIR));
		mvaddch(targetCursor.y,targetCursor.x,'X');
		attroff(COLOR_PAIR(HPBARMISSING_PAIR));

		// if the cursor is on a monster then display the monster's name
		const int distance = player->get_tile_distance(targetCursor);
		if (game.map->is_in_fov(targetCursor))
		{
			const auto& actor = game.map->get_actor(targetCursor);
			// and actor is not an item
			if (actor != nullptr)
			{
				mvprintw(0, 0, actor->actorData.name.c_str());
				// print the monster's stats
				mvprintw(1, 0, "HP: %d/%d", actor->destructible->hp, actor->destructible->hpMax);
				mvprintw(2, 0, "AC: %d", actor->destructible->dr);
				mvprintw(3, 0, "Roll: %s", actor->attacker->roll.data());
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
			targetCursor.y--;
			break;

		case KEY_DOWN:
			// move the selection cursor down
			targetCursor.y++;
			break;

		case KEY_LEFT:
			// move the selection cursor left
			targetCursor.x--;
			break;

		case KEY_RIGHT:
			// move the selection cursor right
			targetCursor.x++;
			break;
			
		case 10:
			// if the key enter is pressed then select the target
			// and return the target position
			// if the target is a monster then attack it
		{
			if (game.map->is_in_fov(targetCursor))
			{
				const auto& actor = game.map->get_actor(targetCursor);
				if (actor)
				{
					player->attacker->attack(*player, *actor);
					run = false;
				}
			}
			game.gameStatus = GameStatus::NEW_TURN;
		}
		break;

		case 'r':
		case 27:
			// if the key escape is pressed then cancel the target selection
			run = false;
			break;
			
		default:
			break;
		} // end of switch (key)

	} // end of while (run)
	clear();
}

void Game::load_all()
{
	// Use JSON to load the game
	game.gameInit = true;

	std::ifstream file("game.sav");
	if (file.is_open())
	{
		json j;
		file >> j;

		// Load the map
		map->load(j);

		// Load the rooms
		if (j.contains("rooms") && j["rooms"].is_array())
		{
			for (const auto& roomData : j["rooms"])
			{
				rooms.push_back(Vector2D{ roomData["y"], roomData["x"] });
			}
		}

		// Load the player
		if (j.contains("player"))
		{
			player->load(j["player"]);
		}

		// Load the stairs
		if (j.contains("stairs"))
		{
			stairs->load(j["stairs"]);
		}

		// Load the other creatures (actors)
		if (j.contains("creatures") && j["creatures"].is_array())
		{
			for (const auto& creatureData : j["creatures"])
			{
				auto creature = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ ' ', "Unnamed", WHITE_PAIR });
				creature->load(creatureData); // Load creature data
				creatures.push_back(std::move(creature));
			}
		}

		// Load items
		if (j.contains("items") && j["items"].is_array())
		{
			for (const auto& itemData : j["items"])
			{
				auto item = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ ' ', "Unnamed", WHITE_PAIR });
				item->load(itemData);
				container->inv.push_back(std::move(item));
			}
		}

		// Load the message log
		if (j.contains("gui"))
		{
			gui->load(j["gui"]);
		}
	}
	else
	{
		game.init(); // If loading fails, reinitialize the game
		game.log("Error: Could not open save file. Game initialized with default settings.");
	}
}

void Game::save_all()
{
	// Use JSON to save the game
	std::ofstream file("game.sav");
	if (file.is_open())
	{
		json j;

		// Save the map
		map->save(j);

		// Save game.rooms
		j["rooms"] = json::array();
		for (const auto& room : rooms)
		{
			j["rooms"].push_back({ {"x", room.x}, {"y", room.y} });
		}

		// Save the player
		json playerJson;
		player->save(playerJson);
		j["player"] = playerJson;

		// Save the stairs
		json stairsJson;
		stairs->save(stairsJson);
		j["stairs"] = stairsJson;

		// Save the other creatures (actors)
		j["creatures"] = json::array(); // Array to hold all creatures
		for (const auto& creature : creatures)
		{
			if (creature)
			{
				json creatureJson;
				creature->save(creatureJson); // Save each creature individually
				j["creatures"].push_back(creatureJson); // Add to array
			}
		}

		// Save items
		j["items"] = json::array();
		for (const auto& item : container->inv)
		{
			if (item)
			{
				json itemJson;
				item->save(itemJson);
				j["items"].push_back(itemJson);
			}
		}

		// Save the message log
		json guiJson;
		gui->save(guiJson);
		j["gui"] = guiJson;

		// Write the JSON data to the file
		file << j.dump(4); // Pretty print with an indentation of 4 spaces
		file.close();
	}
	else
	{
		game.log("Error occurred while saving the game.");
	}
}

void Game::next_level()
{
	dungeonLevel++; // increment the dungeon level
	game.message(WHITE_PAIR, "You take a moment to rest, and recover your strength.",true); // present a message to the player
	player->destructible->heal(player->destructible->hpMax / 2); // heal the player
	game.message(WHITE_PAIR, "deeper into the heart of the dungeon...", true); // present a message to the player (the order is reversed)
	game.message(WHITE_PAIR, "After a rare moment of peace, you descend",true);
	game.message(WHITE_PAIR, std::format("You are now on level {}", dungeonLevel), true);
	map->regenerate(); // create a new map
	gameStatus = GameStatus::STARTUP; // set the game status to STARTUP because we need to recompute the FOV 
}

Creature* Game::get_actor(Vector2D pos) const noexcept
{
	for (const auto& actor : creatures)
	{
		if (actor->position == pos)
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
		mvwprintw(stats, 7, 1, "DR: %d", player->destructible->dr);
		mvwprintw(stats, 8, 1, "Health: %d/%d", player->destructible->hp, player->destructible->hpMax);
		
		wrefresh(stats);

		const int key = getch();
		switch (key)
		{

		case 'a':
		{
			//const int roll = d.d4();
			//player->attacker->dmg += d.d4();
			break;
		}

		case 'd':
		{
			/*player->destructible->dr += 1;*/
			break;
		}

		case 'h':
		{
			//const int roll = d.d8();
			//player->destructible->hpMax += d.d8();
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

	auto run{ true };
	while (run == true)
	{
		// display the player stats
		// using a dnd character sheet
		// based on https://wiki.roll20.net/ADnD_2nd_Edition_Character_sheet

		mvwprintw(character_sheet, 1, 1, "Name: %s", player->actorData.name.c_str()); // display the player name
		mvwprintw(character_sheet, 2, 1, "Class: %s", player->playerClass.c_str()); // display the player class
		mvwprintw(character_sheet, 3, 1, "Kit: "); // display the class kit
		mvwprintw(character_sheet, 4, 1, "Level: %d", player->playerLevel); // display the player level
		mvwprintw(character_sheet, 5, 1, "Experience: %d", player->destructible->xp); // display the player experience
		mvwprintw(character_sheet, 6, 1, "Alignment: "); // display the player alignment
		// add character details on the right side
		mvwprintw(character_sheet, 1, 60, "Race: %s", player->playerRace.c_str()); // display the player race
		mvwprintw(character_sheet, 2, 60, "Gender: %s", player->gender.c_str()); // display gender
		mvwprintw(character_sheet, 3, 60, "Hair Color: "); // display hair color
		mvwprintw(character_sheet, 4, 60, "Eye Color: "); // display eye color
		mvwprintw(character_sheet, 5, 60, "Complexion: "); // display complexion
		mvwprintw(character_sheet, 6, 60, "Features: "); // display features 
		mvwprintw(character_sheet, 7, 60, "Homeland: "); // display homeland
		mvwprintw(character_sheet, 8, 60, "Deity: "); // display deity
		mvwprintw(character_sheet, 9, 60, "Vision: "); // display vision
		mvwprintw(character_sheet, 10, 60, "Secondary Skills: "); // display secondary skills
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
	for (const auto& actor : game.creatures)
	{
		// print the actor's name
		mvprintw(actor->position.y, actor->position.x, actor->actorData.name.c_str());
	}
}

void Game::log(std::string_view message) const
{
	if (debugMode)
	{
		std::clog << message << "\n";
		std::cout << message << "\n";
	}
}

void Game::message(int color, const std::string& text, bool isComplete = false)
{
	// store message in game
	messageToDisplay = text;
	messageColor = color;

	// Always append the message part to attackMessageParts
	attackMessageParts.push_back(LogMessage{ color, text });

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

void Game::handle_ranged_attack() {

	// Enter targeting mode
	Vector2D targetPos = targeting.select_target(player->position, 4);

	// If a valid target was selected
	if (targetPos.x != -1 && targetPos.y != -1) {
		// Process the ranged attack (including projectile animation)
		targeting.process_ranged_attack(*player, targetPos);
	}

}

// end of file: Game.cpp
