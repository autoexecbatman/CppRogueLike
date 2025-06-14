// file: Game.cpp
#include <iostream>
#include <fstream>
#include <curses.h>
#include <algorithm> // for std::remove in sendToBack(Actor*)
#include <ranges>
#include <random>
#include <climits>
#include <cassert>
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
#include "Web.h"
#include "Armor.h"

// added for DEBUGING AT PLAYER FEET
#include "ActorTypes/Healer.h"
#include "ActorTypes/Fireball.h"
#include "ActorTypes/LightningBolt.h"

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

	// REMOVED: Debug weapons no longer added
	// game.add_debug_weapons_at_player_feet();

	//==LOG==
	game.log("game.init() was called!");
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
	// INCREASED SPAWN RATE - add a new monster every 2 turns (was 5)
	if (game.time % 2 == 0)
	{
		// INCREASED MONSTER LIMIT - if there are less than 10 monsters on the map (was 6)
		if (creatures.size() < 10)
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

	//==DRAW==
	game.log("Running render...");
	gui.gui_render(); // render the gui
	game.render(); // render map and actors to the screen
	game.log("Render OK.");
	
	// Check for menus AFTER rendering so positions are updated
	if (!game.menus.empty())
	{
		game.windowState = Game::WindowState::MENU;
		return;
	}
}

void Game::update()
{
	if (gameStatus == GameStatus::VICTORY)
	{
		game.log("Player has won the game!");
		game.appendMessagePart(FIREBALL_PAIR, "Congratulations!");
		game.appendMessagePart(WHITE_PAIR, " You have obtained the ");
		game.appendMessagePart(FIREBALL_PAIR, "Amulet of Yendor");
		game.appendMessagePart(WHITE_PAIR, " and escaped the dungeon!");
		game.finalizeMessage();

		// Display a victory message and wait for a keypress
		WINDOW* victoryWin = newwin(10, 50, (LINES / 2) - 5, (COLS / 2) - 25);
		box(victoryWin, 0, 0);
		mvwprintw(victoryWin, 2, 10, "VICTORY!");
		mvwprintw(victoryWin, 4, 5, "You have won the game!");
		mvwprintw(victoryWin, 6, 5, "Press any key to exit...");
		wrefresh(victoryWin);

		getch(); // Wait for any key
		delwin(victoryWin);

		run = false; // End the game
	}

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
		// Remove destroyed objects
		auto isNull = [](const auto& obj) { return !obj; };
		std::erase_if(objects, isNull);

		game.update_creatures(creatures);
		game.spawn_creatures();

		for (const auto& creature : creatures) {
			if (creature && creature->destructible) {
				creature->destructible->update_constitution_bonus(*creature);
			}
		}

		// Don't forget the player
		if (player && player->destructible) {
			player->destructible->update_constitution_bonus(*player);
		}

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

	// Render any objects (like webs)
	for (const auto& obj : objects)
	{
		if (obj)
		{
			obj->render();
		}
	}

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
		// EMSCRIPTEN FIX: Removed clear() - causes screen corruption in web builds
		// Use selective rendering instead

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
			// EMSCRIPTEN FIX: Clean exit without screen corruption
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

// this function is deprecated
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
		// EMSCRIPTEN FIX: Removed clear() - causes screen corruption in web builds

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
				mvprintw(2, 0, "AC: %d", actor->destructible->armorClass);
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
			// EMSCRIPTEN FIX: Clean exit without screen corruption
			run = false;
			break;
			
		default:
			break;
		} // end of switch (key)

	} // end of while (run)
	// EMSCRIPTEN FIX: Removed problematic clear() at function end
	// Game rendering will handle screen updates
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

	// Create window for level up display
	WINDOW* statsWindow = newwin(
		22, // height
		60, // width
		2,  // y
		10  // x
	);

	box(statsWindow, 0, 0);

	// Title
	wattron(statsWindow, A_BOLD);
	mvwprintw(statsWindow, 1, 20, "LEVEL UP: %s", player->playerClass.c_str());
	wattroff(statsWindow, A_BOLD);

	// Display basic character info
	mvwprintw(statsWindow, 3, 2, "Name: %s", player->actorData.name.c_str());
	mvwprintw(statsWindow, 3, 30, "Race: %s", player->playerRace.c_str());
	mvwprintw(statsWindow, 4, 2, "Level: %d -> %d", xpLevel - 1, xpLevel);
	mvwprintw(statsWindow, 4, 30, "Experience: %d", player->destructible->xp);

	// Store old stats
	int oldHp = player->destructible->hp;
	int oldHpMax = player->destructible->hpMax;
	int oldDr = player->destructible->dr;
	int oldThac0 = player->destructible->thaco;

	// we calculate inside the function to be able to display the change from old THAC0
	game.player->calculate_thaco();

	// Calculate and apply HP gains based on class and Constitution
	int hpGain = 0;
	int hpAdj = game.constitutionAttributes[player->constitution - 1].HPAdj;

	// Calculate HP gain by class
	switch (player->playerClassState)
	{
	case Player::PlayerClassState::FIGHTER:
		hpGain = d.d10();
		break;
	case Player::PlayerClassState::ROGUE:
		hpGain = d.d6();
		break;
	case Player::PlayerClassState::CLERIC:
		hpGain = d.d8();
		break;
	case Player::PlayerClassState::WIZARD:
		hpGain = d.d4();
		break;
	default:
		hpGain = d.d8(); // Default fallback
		break;
	}

	// Ensure minimum HP gain (1 per level according to 2e rules)
	hpGain = std::max(1, hpGain);

	// Apply HP gain
	player->destructible->hpMax += hpGain + hpAdj;
	player->destructible->hp += hpGain + hpAdj;
	player->destructible->hpBase += hpGain;

	// Update player level
	player->playerLevel = xpLevel;

	// Determine if DR should improve (primarily fighters)
	bool drImproved = false;
	if ((player->playerClassState == Player::PlayerClassState::FIGHTER && xpLevel % 3 == 0) ||
		(xpLevel % 4 == 0)) { // Other classes get DR increase every 4 levels
		player->destructible->dr += 1;
		drImproved = true;
	}

	// Display stat changes
	mvwprintw(statsWindow, 6, 2, "IMPROVEMENTS:");

	// HP improvement
	wattron(statsWindow, COLOR_PAIR(HPBARFULL_PAIR));
	mvwprintw(statsWindow, 7, 4, "Hit Points: %d/%d -> %d/%d (+%d)",
		oldHp, oldHpMax, player->destructible->hp, player->destructible->hpMax, hpGain);
	wattroff(statsWindow, COLOR_PAIR(HPBARFULL_PAIR));

	// THAC0 difference
	int difference = player->destructible->thaco - oldThac0;

	// THAC0 improvement
	if (oldThac0 > game.player->destructible->thaco)
	{
		wattron(statsWindow, COLOR_PAIR(WHITE_PAIR));
		mvwprintw(statsWindow, 8, 4, "THAC0: %d -> %d (Improved by %d)",
			oldThac0, game.player->destructible->thaco, difference);
		wattroff(statsWindow, COLOR_PAIR(WHITE_PAIR));
	}
	else {
		mvwprintw(statsWindow, 8, 4, "THAC0: %d (No Change)", game.player->destructible->thaco);
	}

	// Damage Reduction improvement
	if (drImproved) {
		wattron(statsWindow, COLOR_PAIR(TROLL_PAIR));
		mvwprintw(statsWindow, 9, 4, "Damage Reduction: %d -> %d (+1)",
			oldDr, player->destructible->dr);
		wattroff(statsWindow, COLOR_PAIR(TROLL_PAIR));
	}
	else {
		mvwprintw(statsWindow, 9, 4, "Damage Reduction: %d (No Change)", player->destructible->dr);
	}

	// Class-specific benefits
	mvwprintw(statsWindow, 11, 2, "CLASS BENEFITS:");

	switch (player->playerClassState)
	{
	case Player::PlayerClassState::FIGHTER:
		mvwprintw(statsWindow, 12, 4, "- Improved combat prowess");
		mvwprintw(statsWindow, 13, 4, "- Superior hit point gain (d10)");
		if (xpLevel % 3 == 0) {
			mvwprintw(statsWindow, 14, 4, "- Enhanced armor use (+1 DR)");
		}
		break;

	case Player::PlayerClassState::ROGUE:
		mvwprintw(statsWindow, 12, 4, "- Improved thief skills");
		mvwprintw(statsWindow, 13, 4, "- Enhanced stealth capabilities");
		if (xpLevel % 4 == 0) {
			mvwprintw(statsWindow, 14, 4, "- Better trap avoidance (+1 DR)");
		}
		break;

	case Player::PlayerClassState::CLERIC:
		mvwprintw(statsWindow, 12, 4, "- Enhanced divine favor");
		mvwprintw(statsWindow, 13, 4, "- Improved healing capabilities");
		if (player->wisdom >= 13 && xpLevel % 2 == 0) {
			mvwprintw(statsWindow, 14, 4, "- Additional spell slot (Wisdom bonus)");
		}
		break;

	case Player::PlayerClassState::WIZARD:
		mvwprintw(statsWindow, 12, 4, "- Expanded arcane knowledge");
		mvwprintw(statsWindow, 13, 4, "- New spell research opportunities");
		if (player->intelligence >= 13 && xpLevel % 2 == 0) {
			mvwprintw(statsWindow, 14, 4, "- Additional spell slot (Intelligence bonus)");
		}
		break;

	default:
		mvwprintw(statsWindow, 12, 4, "- General combat improvement");
		break;
	}

	// Prompt specifically for space bar
	mvwprintw(statsWindow, 18, 15, "Press SPACE BAR to continue...");
	wrefresh(statsWindow);

	// Wait for space bar
	int ch;
	do {
		ch = getch();
	} while (ch != ' '); // Only accept space bar;

	// Clean up
	delwin(statsWindow);
	clear();

	// Force game status to trigger a full update cycle
	gameStatus = GameStatus::NEW_TURN;
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
		// Calculate XP needed for next level
		int currentXP = player->destructible->xp;
		int nextLevelXP = player->ai->get_next_level_xp(*player);
		int xpNeeded = nextLevelXP - currentXP;
		float progressPercent = (float)currentXP / nextLevelXP * 100.0f;

		// Display the player stats
		mvwprintw(character_sheet, 1, 1, "Name: %s", player->actorData.name.c_str());
		mvwprintw(character_sheet, 2, 1, "Class: %s", player->playerClass.c_str());
		mvwprintw(character_sheet, 3, 1, "Race: %s", player->playerRace.c_str());
		mvwprintw(character_sheet, 4, 1, "Level: %d", player->playerLevel);

		// Enhanced XP display with progress to next level
		mvwprintw(character_sheet, 5, 1, "Experience: %d / %d (%.1f%% to level %d)",
			currentXP, nextLevelXP, progressPercent, player->playerLevel + 1);
		mvwprintw(character_sheet, 6, 1, "XP needed for next level: %d", xpNeeded);

		// Get strength modifiers from attributes table
		int strHitMod = 0;
		int strDmgMod = 0;
		if (player->strength > 0 && player->strength <= strengthAttributes.size()) {
			strHitMod = strengthAttributes[player->strength - 1].hitProb;
			strDmgMod = strengthAttributes[player->strength - 1].dmgAdj;
		}

		// Add character attributes with modifiers
		mvwprintw(character_sheet, 8, 1, "Attributes:");

		// Display Strength with modifiers
		wattron(character_sheet, A_BOLD);
		mvwprintw(character_sheet, 9, 3, "Strength: %d", player->strength);
		wattroff(character_sheet, A_BOLD);

		// Show hit and damage modifiers from strength
		if (strHitMod != 0 || strDmgMod != 0) {
			wattron(character_sheet, COLOR_PAIR((strHitMod >= 0) ? HPBARFULL_PAIR : HPBARMISSING_PAIR));
			mvwprintw(character_sheet, 9, 20, "To Hit: %+d", strHitMod);
			wattroff(character_sheet, COLOR_PAIR((strHitMod >= 0) ? HPBARFULL_PAIR : HPBARMISSING_PAIR));

			wattron(character_sheet, COLOR_PAIR((strDmgMod >= 0) ? HPBARFULL_PAIR : HPBARMISSING_PAIR));
			mvwprintw(character_sheet, 9, 35, "Damage: %+d", strDmgMod);
			wattroff(character_sheet, COLOR_PAIR((strDmgMod >= 0) ? HPBARFULL_PAIR : HPBARMISSING_PAIR));
		}

		// Display Constitution with HP bonus effect
		int conBonus = 0;
		if (player->constitution >= 1 && player->constitution <= constitutionAttributes.size()) {
			conBonus = constitutionAttributes[player->constitution - 1].HPAdj;
		}

		mvwprintw(character_sheet, 10, 3, "Dexterity: %d", player->dexterity);

		if (conBonus != 0) {
			wattron(character_sheet, COLOR_PAIR((conBonus > 0) ? HPBARFULL_PAIR : HPBARMISSING_PAIR));
			mvwprintw(character_sheet, 11, 3, "Constitution: %d (%+d HP per level)",
				player->constitution, conBonus);
			wattroff(character_sheet, COLOR_PAIR((conBonus > 0) ? HPBARFULL_PAIR : HPBARMISSING_PAIR));
		}
		else {
			mvwprintw(character_sheet, 11, 3, "Constitution: %d", player->constitution);
		}

		mvwprintw(character_sheet, 12, 3, "Intelligence: %d", player->intelligence);
		mvwprintw(character_sheet, 13, 3, "Wisdom: %d", player->wisdom);
		mvwprintw(character_sheet, 14, 3, "Charisma: %d", player->charisma);

		// Add combat stats with Constitution effect
		mvwprintw(character_sheet, 16, 1, "Combat Statistics:");

		// Calculate base HP and Con bonus
		int baseHP = player->destructible->hpBase;
		int conBonusTotal = player->destructible->hpMax - baseHP;

		// Show HP with Constitution effect
		if (conBonusTotal != 0)
		{
			mvwprintw(character_sheet, 17, 3, "HP: %d/%d ",
				player->destructible->hp, player->destructible->hpMax);

			wattron(character_sheet, COLOR_PAIR((conBonusTotal > 0) ? HPBARFULL_PAIR : HPBARMISSING_PAIR));
			wprintw(character_sheet, "(%d base %+d Con bonus)",
				baseHP, conBonusTotal);
			wattroff(character_sheet, COLOR_PAIR((conBonusTotal > 0) ? HPBARFULL_PAIR : HPBARMISSING_PAIR));
		}
		else
		{
			mvwprintw(character_sheet, 17, 3, "HP: %d/%d",
				player->destructible->hp, player->destructible->hpMax);
		}

		// Display attack bonus from strength if applicable
		if (strHitMod != 0 || strDmgMod != 0) {
			mvwprintw(character_sheet, 18, 3, "Attack Bonuses: ");
			if (strHitMod != 0) {
				wattron(character_sheet, COLOR_PAIR((strHitMod > 0) ? HPBARFULL_PAIR : HPBARMISSING_PAIR));
				wprintw(character_sheet, "%+d to hit ", strHitMod);
				wattroff(character_sheet, COLOR_PAIR((strHitMod > 0) ? HPBARFULL_PAIR : HPBARMISSING_PAIR));
			}
			if (strDmgMod != 0) {
				wattron(character_sheet, COLOR_PAIR((strDmgMod > 0) ? HPBARFULL_PAIR : HPBARMISSING_PAIR));
				wprintw(character_sheet, "%+d to damage", strDmgMod);
				wattroff(character_sheet, COLOR_PAIR((strDmgMod > 0) ? HPBARFULL_PAIR : HPBARMISSING_PAIR));
			}
		}

		mvwprintw(character_sheet, 19, 3, "Armor Class: %d", player->destructible->armorClass);
		mvwprintw(character_sheet, 20, 3, "THAC0: %d", player->destructible->thaco);

		// Display dexterity bonuses
		if (player->dexterity > 0 && player->dexterity <= dexterityAttributes.size()) {
			// Show missile attack adjustment
			int missileAdj = dexterityAttributes[player->dexterity - 1].MissileAttackAdj;
			mvwprintw(character_sheet, 21, 3, "Ranged Attack Bonus: %d", missileAdj);

			// Show defensive adjustment
			int defensiveAdj = dexterityAttributes[player->dexterity - 1].DefensiveAdj;
			mvwprintw(character_sheet, 22, 3, "Defensive Adjustment: %d AC", defensiveAdj);
		}

		// Equipped weapon info with color
		wattron(character_sheet, COLOR_PAIR(GOBLIN_PAIR));
		mvwprintw(character_sheet, 24, 1, "Equipment:");

		if (player->weaponEquipped != "None") {
			mvwprintw(character_sheet, 25, 3, "Weapon: %s (%s damage)",
				player->weaponEquipped.c_str(), player->attacker->roll.c_str());

			// Show if weapon is ranged
			if (player->has_state(ActorState::IS_RANGED)) {
				wattron(character_sheet, COLOR_PAIR(LIGHTNING_PAIR));
				mvwprintw(character_sheet, 25, 40, "[Ranged]");
				wattroff(character_sheet, COLOR_PAIR(LIGHTNING_PAIR));
			}

			// Show effective damage with strength bonus
			if (strDmgMod != 0) {
				wattron(character_sheet, COLOR_PAIR(FIREBALL_PAIR));
				mvwprintw(character_sheet, 26, 3, "Effective damage: %s %+d",
					player->attacker->roll.c_str(), strDmgMod);
				wattroff(character_sheet, COLOR_PAIR(FIREBALL_PAIR));
			}
		}
		else {
			mvwprintw(character_sheet, 25, 3, "Weapon: Unarmed (D2 damage)");

			// Show effective unarmed damage with strength bonus
			if (strDmgMod != 0) {
				wattron(character_sheet, COLOR_PAIR(FIREBALL_PAIR));
				mvwprintw(character_sheet, 26, 3, "Effective damage: D2 %+d", strDmgMod);
				wattroff(character_sheet, COLOR_PAIR(FIREBALL_PAIR));
			}
		}
		wattroff(character_sheet, COLOR_PAIR(GOBLIN_PAIR));

		// Add gold and other stats on the right side
		mvwprintw(character_sheet, 9, 60, "Gender: %s", player->gender.c_str());
		mvwprintw(character_sheet, 10, 60, "Gold: %d", player->gold);
		mvwprintw(character_sheet, 11, 60, "Hunger: %s",
			game.hunger_system.get_hunger_state_string().c_str());

		// Add Constitution details panel on the right side
		mvwprintw(character_sheet, 13, 60, "Constitution Effects:");

		if (player->constitution >= 1 && player->constitution <= constitutionAttributes.size()) {
			const auto& conAttr = constitutionAttributes[player->constitution - 1];

			mvwprintw(character_sheet, 14, 62, "HP Adjustment: %+d per level", conAttr.HPAdj);
			mvwprintw(character_sheet, 15, 62, "System Shock: %d%%", conAttr.SystemShock);
			mvwprintw(character_sheet, 16, 62, "Resurrection Survival: %d%%", conAttr.ResurrectionSurvival);
			mvwprintw(character_sheet, 17, 62, "Poison Save Modifier: %+d", conAttr.PoisonSave);

			if (conAttr.Regeneration > 0) {
				mvwprintw(character_sheet, 18, 62, "Regeneration: %d HP per turn", conAttr.Regeneration);
			}
		}

		// Add strength details panel on the right side
		mvwprintw(character_sheet, 20, 60, "Strength Effects:");

		if (player->strength >= 1 && player->strength <= strengthAttributes.size()) {
			const auto& strAttr = strengthAttributes[player->strength - 1];

			mvwprintw(character_sheet, 21, 62, "Hit Probability Adj: %+d", strAttr.hitProb);
			mvwprintw(character_sheet, 22, 62, "Damage Adjustment: %+d", strAttr.dmgAdj);
			mvwprintw(character_sheet, 23, 62, "Weight Allowance: %d lbs", strAttr.wgtAllow);
			mvwprintw(character_sheet, 24, 62, "Max Press: %d lbs", strAttr.maxPress);
			mvwprintw(character_sheet, 25, 62, "Open Doors: %d/6", strAttr.openDoors);
		}

		mvwprintw(character_sheet, 28, 1, "Press any key to close...");

		wrefresh(character_sheet);

		const int key = getch();
		// If any key was pressed then exit the loop
		if (key != ERR) {
			run = false;
		}
	}
	delwin(character_sheet);
	//clear();
	// Redraw screen
	clear();
	/*game.render();*/
	refresh();
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
		case 'm':  // For "monsters"
			map->display_spawn_rates();
			break;
		case 'i':  // For "items"
			map->display_item_distribution();
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

void Game::handle_ranged_attack()
{
	// When attackMode is true, we require a ranged weapon for attacks
	// When attackMode is false, we're just examining and don't require a ranged weapon

	// Enter targeting mode with appropriate requirements
	Vector2D targetPos = targeting.select_target(player->position, 4);

	// If a valid target was selected and we're in attack mode
	if (targetPos.x != -1 && targetPos.y != -1) {
		// Process the ranged attack (including projectile animation)
		targeting.process_ranged_attack(*player, targetPos);
	}
}


void Game::display_help() noexcept
{
	WINDOW* help_window = newwin(
		30, // height
		70, // width
		0,  // y
		0   // x
	);
	box(help_window, 0, 0);
	refresh();
	bool run{ true };
	while (run == true)
	{
		mvwprintw(help_window, 1, 1, "=== CONTROLS ===");
		mvwprintw(help_window, 3, 1, "Movement: Arrow keys, WASD (W/A/S/D)");
		mvwprintw(help_window, 4, 1, "Diagonal: (Q/E/Z/C)");
		mvwprintw(help_window, 5, 1, "Wait: 'h'");
		mvwprintw(help_window, 6, 1, "Pick up item: 'p'");
		mvwprintw(help_window, 7, 1, "Drop item: 'l'");
		mvwprintw(help_window, 8, 1, "Inventory: 'i'");
		mvwprintw(help_window, 9, 1, "Character sheet: '@'");
		mvwprintw(help_window, 10, 1, "Ranged attack or look around: 't' (requires ranged weapon to shoot)");
		mvwprintw(help_window, 12, 1, "Open door: 'o'");
		mvwprintw(help_window, 13, 1, "Close door: 'k'");
		mvwprintw(help_window, 14, 1, "Rest: 'r' (recovers health but costs food)");
		mvwprintw(help_window, 15, 1, "Descend stairs: '>'");
		mvwprintw(help_window, 16, 1, "Help: '?'");
		mvwprintw(help_window, 17, 1, "Menu: 'Esc'");
		mvwprintw(help_window, 18, 1, "Quit: '~'");
		mvwprintw(help_window, 19, 1, "=== RESTING ===");
		mvwprintw(help_window, 20, 1, "- Resting recovers 20 percent of your maximum health");
		mvwprintw(help_window, 21, 1, "- You cannot rest when enemies are nearby (within 5 tiles)");
		mvwprintw(help_window, 22, 1, "- Resting increases hunger, so make sure to have food");
		mvwprintw(help_window, 23, 1, "- You cannot rest if you're starving");
		mvwprintw(help_window, 28, 1, "Press any key to close this window");
		wrefresh(help_window);
		const int key = getch();
		// If any key was pressed then exit the loop
		if (key != ERR) {
			run = false;
		}
	}
	delwin(help_window);
	clear();
	refresh();
}

Web* Game::findWebAt(Vector2D position)
{
	for (const auto& obj : objects)
	{
		if (obj && obj->position == position &&
			obj->actorData.name == "spider web")
		{
			return dynamic_cast<Web*>(obj.get());
		}
	}
	return nullptr;
}

void Game::add_debug_weapons_at_player_feet()
{
	// Only add weapons if player exists
	if (!player) {
		log("Error: Cannot add debug weapons - player not initialized");
		return;
	}

	// Create a longsword at player's feet
	auto longsword = std::make_unique<Item>(player->position, ActorData{ '/', "long sword", WHITE_PAIR });
	longsword->pickable = std::make_unique<LongSword>();
	longsword->value = 50; // Set a value for the longsword
	container->add(std::move(longsword));

	// Create a longbow at player's feet
	auto longbow = std::make_unique<Item>(player->position, ActorData{ ')', "longbow", LIGHTNING_PAIR });
	longbow->pickable = std::make_unique<Longbow>();
	longbow->value = 70; // Set a value for the longbow
	container->add(std::move(longbow));

	// Add a health potion for good measure
	auto healthPotion = std::make_unique<Item>(player->position, ActorData{ '!', "health potion", HPBARMISSING_PAIR });
	healthPotion->pickable = std::make_unique<Healer>(10);
	healthPotion->value = 25;
	container->add(std::move(healthPotion));

	// Add a scroll of fireball at player's feet
	auto scrollOfFireball = std::make_unique<Item>(player->position, ActorData{ '#', "scroll of fireball", FIREBALL_PAIR });
	scrollOfFireball->pickable = std::make_unique<Fireball>(3, 12);
	scrollOfFireball->value = 100;
	container->add(std::move(scrollOfFireball));

	auto scrollOfLightning = std::make_unique<Item>(player->position, ActorData{ '#', "scroll of lightning", LIGHTNING_PAIR });
	scrollOfLightning->pickable = std::make_unique<LightningBolt>(5, 20);
	scrollOfLightning->value = 150;
	container->add(std::move(scrollOfLightning));

	auto leatherArmor = std::make_unique<Item>(player->position, ActorData{ '[', "leather armor", DOOR_PAIR });
	leatherArmor->pickable = std::make_unique<LeatherArmor>();
	leatherArmor->value = 30;
	container->add(std::move(leatherArmor));

	auto chainMail = std::make_unique<Item>(player->position, ActorData{ '[', "chain mail", DOOR_PAIR });
	chainMail->pickable = std::make_unique<ChainMail>();
	chainMail->value = 75;
	container->add(std::move(chainMail));

	auto plateMail = std::make_unique<Item>(player->position, ActorData{ '[', "plate mail", DOOR_PAIR });
	plateMail->pickable = std::make_unique<PlateMail>();
	plateMail->value = 150;
	container->add(std::move(plateMail));

	log("Debug weapons added at player position: " +
		std::to_string(player->position.x) + "," +
		std::to_string(player->position.y));

	// Add a message for the player
	message(WHITE_PAIR, "Debug weapons and armor placed at your feet.", true);

}

// EMSCRIPTEN COMPATIBILITY FUNCTIONS
void Game::safe_screen_clear()
{
#ifdef EMSCRIPTEN
	// For Emscripten: Don't use clear(), just redraw
	restore_game_display();
#else
	// For native builds: Use normal clear
	clear();
	refresh();
#endif
}

void Game::force_screen_refresh()
{
	refresh();
	doupdate();  // Force immediate update
}

void Game::restore_game_display()
{
	// Clean redraw of entire game state
	game.render();
	gui->gui_render();
	force_screen_refresh();
}

// end of file: Game.cpp
