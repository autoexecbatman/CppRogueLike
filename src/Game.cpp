// file: Game.cpp
#include <iostream>
#include <fstream>
#include <curses.h>
#include <memory>
#include <string>

#include "Game.h"
#include "Colors/Colors.h"
#include "Factories/ItemCreator.h"

// Debug weapon types
#include "ActorTypes/Healer.h"
#include "ActorTypes/Fireball.h"
#include "ActorTypes/LightningBolt.h"
#include "ActorTypes/Confuser.h"

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
		// Remove destroyed objects - delegated to ObjectManager
		object_manager.cleanup_destroyed_objects(objects);

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

void Game::load_all()
{
	menu_manager.set_game_initialized(true);
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

// displays the actors as names
void Game::wizard_eye() noexcept
{
	for (const auto& actor : game.creatures)
	{
		// print the actor's name
		mvprintw(actor->position.y, actor->position.x, actor->actorData.name.c_str());
	}
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
