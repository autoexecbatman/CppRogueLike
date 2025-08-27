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
#include "Items/Items.h"
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
#include "Objects/Web.h"
#include "Factories/ItemCreator.h"
#include "Items/Armor.h"
#include "Systems/LevelUpSystem.h"
#include "UI/LevelUpUI.h"
#include "UI/CharacterSheetUI.h"

// added for DEBUGING AT PLAYER FEET
#include "ActorTypes/Healer.h"
#include "ActorTypes/Fireball.h"
#include "ActorTypes/LightningBolt.h"
#include "ActorTypes/Confuser.h"

#include "Factories/ItemCreator.h"

//==INIT==
// When the Game is created, 
// We don't know yet if we have to generate a new map or load a previously saved one.
// Will be called in load_all() when loading and menu() for new game
void Game::init()
{
	//==INIT==
	weapons = load_weapons();
	strengthAttributes = load_strength_attributes();

	//==STAIRS==
	stairs = std::make_unique<Stairs>(Vector2D{ 0,0 });

	//==MAP==
	map.init(true); // with actors true

	//==GameStatus==
	// we set gameStatus to STARTUP because we want to compute the fov only once
	gameStatus = GameStatus::STARTUP;
	log("GameStatus::STARTUP");

	// Add debug weapons and items at player feet
	add_debug_weapons_at_player_feet();

	//==LOG==
	log("game.init() was called!");
}

void Game::handle_menus()
{
	if (!menus.empty())
	{
		bool menuWasPopped = false;
		
		menus.back()->menu();
		// if back is pressed, pop the menu
		if (menus.back()->back)
		{
			menus.pop_back();
			menuWasPopped = true;
			if (!menus.empty())
			{
				menus.back()->menu_set_run_true();
			}
		}

		if (!menus.empty() && !menus.back()->run)
		{
			menus.pop_back();
			menuWasPopped = true;
		}

		// If we just closed a menu and returned to game, restore display
		if (menuWasPopped && menus.empty() && gameWasInit)
		{
			restore_game_display();
		}

		shouldTakeInput = false;
	}
}

void Game::handle_gameloop(Gui& gui, int loopNum)
{
	if (!gameWasInit)
	{
		init();
		gameWasInit = true;
	}
	//==DEBUG==
	log("//====================LOOP====================//");
	log("Loop number: " + std::to_string(loopNum) + "\n");

	//==INIT_GUI==
	// GUI initialization is now handled in STARTUP completion
	// This ensures it happens after racial bonuses are applied

	//==INPUT==
	input_handler.reset_key(); // reset the keyPress so it won't get stuck in a loop
	if (shouldTakeInput)
	{
		input_handler.key_store();
		input_handler.key_listen();
	}
	shouldTakeInput = true; // reset shouldInput to reset the flag

	//==UPDATE==
	log("Running update...");
	update(); // update map and actors positions
	gui.gui_update(); // update the gui
	log("Update OK.");

	//==DRAW==
	log("Running render...");
	// Render game content first, then GUI on top
	render(); // render map and actors to the screen
	// Render GUI if it's initialized - AFTER game render so it's not overwritten
	if (gui.guiInit) {
		// Ensure GUI has latest data before rendering
		gui.gui_update();
		gui.gui_render(); // render the gui
	}
	// Call the same restore function that inventory uses
	restore_game_display();
	log("Render OK.");
	
	// Check for menus AFTER rendering so positions are updated
	if (!menus.empty())
	{
		windowState = Game::WindowState::MENU;
		return;
	}
}

void Game::update()
{
	if (gameStatus == GameStatus::VICTORY)
	{
		log("Player has won the game!");
		append_message_part(RED_YELLOW_PAIR, "Congratulations!");
		append_message_part(WHITE_BLACK_PAIR, " You have obtained the ");
		append_message_part(RED_YELLOW_PAIR, "Amulet of Yendor");
		append_message_part(WHITE_BLACK_PAIR, " and escaped the dungeon!");
		finalize_message();

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

	map.update(); // sets tiles to explored
	player->update(); // if moved set to NEW_TURN else IDLE

	if (gameStatus == GameStatus::STARTUP)
	{
		map.compute_fov();
		// Only adjust racial abilities on initial character creation (dungeonLevel 1)
		if (level_manager.get_dungeon_level() == 1)
		{
			player->racial_ability_adjustments();
			// Don't try to restore display here - let normal game flow handle it
		}
		player->calculate_thaco();
		gameStatus = GameStatus::NEW_TURN;
		
		// Initialize GUI now that STARTUP is complete and player stats are finalized
		if (!gui.guiInit)
		{
			gui.gui_init();
			gui.guiInit = true;
			// Immediately update GUI with current player data
			gui.gui_update();
		}
	}

	if (gameStatus == GameStatus::NEW_TURN)
	{
		// Remove destroyed objects
		auto isNull = [](const auto& obj) { return !obj; };
		std::erase_if(objects, isNull);

		update_creatures(creatures);
		spawn_creatures();

		for (const auto& creature : creatures)
		{
			if (creature && creature->destructible)
			{
				creature->destructible->update_constitution_bonus(*creature);
			}
		}

		// Don't forget the player
		if (player && player->destructible)
		{
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
		game.append_message_part(COLOR_RED, "You died! Press any key...");
		game.finalize_message();
		run = false;
	}
}

bool Game::pick_tile(Vector2D* position, int maxRange)
{
	// the target cursor is initialized at the player's position
	Vector2D targetCursor = player->position;
	bool run = true;
	while (run)
	{
		// CRITICAL FIX: Clear screen to remove previous targeting overlays
		clear();
		
		// make the line follow the mouse position
		// if mouse move
		if (input_handler.mouse_moved())
		{
			targetCursor = input_handler.get_mouse_position();
		}
		game.render();

		// first color the player position if the cursor has moved from the player position
		if (targetCursor != player->position)
		{
			mvchgat(player->position.y, player->position.x, 1, A_NORMAL, WHITE_BLACK_PAIR, nullptr);
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
			mvchgat(line.y, line.x, 1, A_STANDOUT, WHITE_BLACK_PAIR, nullptr);
		}

		attron(COLOR_PAIR(WHITE_RED_PAIR));
		mvaddch(targetCursor.y, targetCursor.x, 'X');
		attroff(COLOR_PAIR(WHITE_RED_PAIR));

		// if the cursor is on a monster then display the monster's name
		if (game.map.is_in_fov(targetCursor))
		{
			const auto& actor = game.map.get_actor(targetCursor);
			// CRITICAL FIX: Don't write directly to main screen - this causes bleeding
			// Store info for later display or use a separate window
			if (actor != nullptr)
			{
				// These direct writes to (0,0) cause screen bleeding - commenting out for now
				// mvprintw(0, 0, actor->actorData.name.c_str());
				// mvprintw(1, 0, "HP: %d/%d", actor->destructible->hp, actor->destructible->hpMax);
				// mvprintw(2, 0, "AC: %d", actor->destructible->dr);
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
					mvchgat(tilePosY, tilePosX, 1, A_REVERSE, WHITE_BLUE_PAIR, nullptr);
				}
			}
		}
		refresh();

		// get the key press
		const int key = getch();
		switch (key)
		{
		case KEY_UP:
		case 'w':
		case 'W':
			// move the selection cursor up
			targetCursor.y--;
			break;

		case KEY_DOWN:
		case 's':
		case 'S':
			// move the selection cursor down
			targetCursor.y++;
			break;

		case KEY_LEFT:
		case 'a':
		case 'A':
			// move the selection cursor left
			targetCursor.x--;
			break;

		case KEY_RIGHT:
		case 'd':
		case 'D':
			// move the selection cursor right
			targetCursor.x++;
			break;

		case 'f':
			// if the player presses the 'f' key
			// then the target selection is confirmed
			// and the target coordinates are returned
			
			// first display a message
			game.message(WHITE_BLACK_PAIR, "Target confirmed", true);
			// then return the coordinates
			*position = targetCursor;
			
			// Restore game display before returning
			game.restore_game_display();
			return true;
			break;

		case 10:
			// if the key enter is pressed then select the target
			// and return the target position
			game.message(WHITE_BLACK_PAIR, "Attack confirmed", true);
			// if the target is a monster then attack it
		{
			if (game.map.is_in_fov(targetCursor))
			{
				const auto& actor = game.map.get_actor(targetCursor);
				// and actor is not an item
				if (actor != nullptr)
				{
					player->attacker->attack(*player, *actor);
					// Restore game display after attack
					game.restore_game_display();
					run = false;
				}
			}
		}
		break;
		case 'r':
		case 27:
			game.message(WHITE_BLACK_PAIR, "Target selection canceled", true);
			// Restore game display before exit
			game.restore_game_display();
			run = false;
			break;

		default:break;
		}

	}

	return false;
}

void Game::load_all()
{
	gameWasInit = true;
	if (!state_manager.load_game(
		map,
		rooms,
		*player,
		*stairs,
		creatures,
		*container,
		gui,
		hunger_system,
		level_manager,
		time))
	{
		init();
		log("Error: Could not open save file. Game initialized with default settings.");
	}
	else
	{
		gameStatus = GameStatus::STARTUP;
		log("GameStatus set to STARTUP after loading for FOV computation");
	}
}

void Game::save_all()
{
	try
	{
		state_manager.save_game(
			map,
			rooms,
			*player,
			*stairs,
			creatures,
			*container,
			gui,
			hunger_system,
			level_manager,
			time
		);
	}
	catch (const std::exception& e)
	{
		log("Error occurred while saving the game: " + std::string(e.what()));
	}
}

void Game::display_levelup(int xpLevel) 
{
	// Apply all level up benefits through the new LevelUpSystem
	LevelUpSystem::apply_level_up_benefits(*player, xpLevel);

	// Display the level up screen using the dedicated UI class
	LevelUpUI::display_level_up_screen(*player, xpLevel);
}

// display character sheet
void Game::display_character_sheet() const noexcept
{
	CharacterSheetUI::display_character_sheet(*player);
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
	if (!player)
	{
		log("Error: Cannot add debug weapons - player not initialized");
		return;
	}

	container->add(ItemCreator::create_long_sword(player->position));
	container->add(ItemCreator::create_longbow(player->position));
	container->add(ItemCreator::create_greatsword(player->position));
	container->add(ItemCreator::create_battle_axe(player->position));
	container->add(ItemCreator::create_great_axe(player->position));
	container->add(ItemCreator::create_war_hammer(player->position));
	container->add(ItemCreator::create_shield(player->position));
	container->add(ItemCreator::create_health_potion(player->position));
	container->add(ItemCreator::create_scroll_fireball(player->position));
	container->add(ItemCreator::create_scroll_lightning(player->position));
	container->add(ItemCreator::create_scroll_confusion(player->position));
	container->add(ItemCreator::create_leather_armor(player->position));
	container->add(ItemCreator::create_chain_mail(player->position));
	container->add(ItemCreator::create_plate_mail(player->position));
	container->add(ItemCreator::create_dagger(player->position));

	log("Debug weapons added at player position: " +
		std::to_string(player->position.x) + "," +
		std::to_string(player->position.y));

	// Add a message for the player
	message(WHITE_BLACK_PAIR, "Debug weapons, two-handed weapons, armor, and shield placed at your feet.", true);

}

// end of file: Game.cpp
