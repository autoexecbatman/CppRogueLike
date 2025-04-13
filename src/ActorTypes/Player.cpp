// file: Player.cpp
#include <gsl/util>
#include <curses.h>
#include "../Game.h"
#include "../Map/Map.h"
#include "../Ai/Ai.h"
#include "../Ai/AiPlayer.h"
#include "../ActorTypes/Player.h"
#include "../Actor/Actor.h"
#include "../Random/RandomDice.h"
#include "../Colors/Colors.h"
#include "../dnd_tables/CalculatedTHAC0s.h"
#include "../Web.h"

Player::Player(Vector2D position) : Creature(position, ActorData{ '@', "Player", WHITE_PAIR })
{
	add_state(ActorState::CAN_SWIM);

	auto roll3d6 = []() { return game.d.d6() + game.d.d6() + game.d.d6(); };
	strength = roll3d6();
	dexterity = roll3d6();
	constitution = roll3d6();
	intelligence = roll3d6();
	wisdom = roll3d6();
	charisma = roll3d6();

	//==PLAYER==
	const int playerHp = 20 + game.d.d10(); // we roll the dice to get the player's hp
	const int playerDr = 1; // the player's damage reduction
	const int playerXp = 0; // the player's experience points
	const int playerAC = 10; // the player's armor class

	gold = 100;

	attacker = std::make_unique<Attacker>("D2");
	destructible = std::make_unique<PlayerDestructible>(
		playerHp,
		playerDr,
		"your corpse",
		playerXp,
		0, // thaco is caluclated from table
		playerAC
	);
	ai = std::make_unique<AiPlayer>();
	container = std::make_unique<Container>(26);
	
}

void Player::racial_ability_adjustments()
{
	// switch player state
	switch (game.player->playerRaceState)
	{
	case Player::PlayerRaceState::HUMAN:
		break;
	case Player::PlayerRaceState::DWARF:
		// write dwarf stuff
		clear();
		mvprintw(0, 0, "You got +1 to constitution and -1 to charisma for being a dwarf.");
		refresh();
		getch();

		game.player->constitution += 1;
		game.player->charisma -= 1;
		break;
	case Player::PlayerRaceState::ELF:
		// write elf stuff
		clear();
		mvprintw(0, 0, "You got +1 to dexterity and -1 to constitution for being an elf.");
		refresh();
		getch();

		game.player->dexterity += 1;
		game.player->constitution -= 1;
		break;
	case Player::PlayerRaceState::GNOME:
		// write gnome stuff
		clear();
		mvprintw(0, 0, "You got +1 to intelligence and -1 to wisdom for being a gnome.");
		refresh();
		getch();

		game.player->intelligence += 1;
		game.player->wisdom -= 1;
		break;
	case Player::PlayerRaceState::HALFELF:
		break;
	case Player::PlayerRaceState::HALFLING:
		// write halfling stuff
		clear();
		mvprintw(0, 0, "You got +1 to dexterity and -1 to strength for being a halfling.");
		refresh();
		getch();

		game.player->dexterity += 1;
		game.player->strength -= 1;
		break;
	default:
		break;
	}
}

void Player::calculate_thaco()
{
	// game.err("Calculating THAC0...");
	// print playerClassState

	CalculatedTHAC0s thaco;
	switch (playerClassState)
	{	
	case PlayerClassState::FIGHTER:
		game.player->destructible->thaco = thaco.getFighter(playerLevel);
		/*game.err(playerClass + "you got THAC0: " + std::to_string(game.player->destructible->thaco));*/
		break;
	case PlayerClassState::ROGUE:
		game.player->destructible->thaco = thaco.getRogue(playerLevel);
		/*game.err(playerClass + "you got THAC0: " + std::to_string(game.player->destructible->thaco));*/
		break;
	case PlayerClassState::CLERIC:
		game.player->destructible->thaco = thaco.getCleric(playerLevel);
		/*game.err(playerClass + "you got THAC0: " + std::to_string(game.player->destructible->thaco));*/
		break;
	case PlayerClassState::WIZARD:
		game.player->destructible->thaco = thaco.getWizard(playerLevel);
		/*game.err(playerClass + "you got THAC0: " + std::to_string(game.player->destructible->thaco));*/
		break;
	case PlayerClassState::NONE:
		break;
	default:
		break;
	}
}

void Player::consume_food(int nutrition) {
	// Decrease hunger by nutrition value
	game.hunger_system.decrease_hunger(nutrition);
}

void Player::render() const noexcept
{
	// First, render the player normally
	Creature::render();
}

bool Player::rest()
{
	// Check if player is already at full health
	if (destructible->hp >= destructible->hpMax)
	{
		game.message(WHITE_PAIR, "You're already at full health.", true);
		return false;
	}

	// Check if enemies are nearby (within a radius of 5 tiles)
	for (const auto& creature : game.creatures)
	{
		if (creature && !creature->destructible->is_dead())
		{
			// Calculate distance to creature
			int distance = get_tile_distance(creature->position);

			// If enemy is within 5 tiles, can't rest
			if (distance <= 5)
			{
				game.message(WHITE_PAIR, "You can't rest with enemies nearby!", true);
				return false;
			}
		}
	}

	// Check if player has enough food (hunger isn't too high)
	if (game.hunger_system.get_hunger_state() == HungerState::STARVING ||
		game.hunger_system.get_hunger_state() == HungerState::DYING)
	{
		game.message(HPBARMISSING_PAIR, "You're too hungry to rest!", true);
		return false;
	}

	// Show resting animation
	animate_resting();

	// Rest - heal 20% of max HP
	int healAmount = std::max(1, destructible->hpMax / 5);
	int amountHealed = destructible->heal(healAmount);

	// Capture the hunger state before and after
	HungerState beforeState = game.hunger_system.get_hunger_state();

	// Consume food (increase hunger)
	const int hungerCost = 50;
	game.hunger_system.increase_hunger(hungerCost);

	HungerState afterState = game.hunger_system.get_hunger_state();

	// Display message with more detail
	game.appendMessagePart(HPBARFULL_PAIR, "You rest and recover ");
	game.appendMessagePart(HPBARFULL_PAIR, std::to_string(amountHealed));
	game.appendMessagePart(HPBARFULL_PAIR, " health");

	if (beforeState != afterState) {
		// If hunger state changed, mention it
		game.appendMessagePart(WHITE_PAIR, ", but you've become ");
		game.appendMessagePart(game.hunger_system.get_hunger_color(), game.hunger_system.get_hunger_state_string().c_str());
		game.appendMessagePart(WHITE_PAIR, ".");
	}
	else {
		game.appendMessagePart(WHITE_PAIR, ", consuming some of your food reserves.");
	}

	game.finalizeMessage();

	// Resting takes time
	game.gameStatus = Game::GameStatus::NEW_TURN;
	return true;
}

void Player::animate_resting()
{
	// Animation frames - Z's of increasing size
	const std::vector<char> restSymbols = { 'z', 'Z', 'Z' };
	const int frames = 3;
	const int delay = 250; // milliseconds between frames

	// Save original screen
	clear();
	game.render();

	// Show animation
	for (int i = 0; i < frames; i++)
	{
		// Draw Zs above player
		attron(COLOR_PAIR(HPBARFULL_PAIR));
		mvaddch(position.y - 1, position.x + i, restSymbols[i]);
		attroff(COLOR_PAIR(HPBARFULL_PAIR));

		refresh();
		napms(delay);
	}

	// Pause briefly to show final frame
	napms(delay * 2);

	// Redraw screen
	clear();
	game.render();
	refresh();
}

void Player::getStuckInWeb(int duration, int strength, Web* web)
{
	webStuckTurns = duration;
	webStrength = strength;
	trappingWeb = web;

	game.message(WHITE_PAIR, "You're caught in a sticky web for " +
		std::to_string(duration) + " turns!", true);
}

bool Player::tryBreakWeb()
{
	// Calculate chance to break free based on strength vs web strength
	int breakChance = 20 + (strength * 5) - (webStrength * 10);

	// Ensure some minimum chance
	breakChance = std::max(10, breakChance);

	// Roll to break free
	if (game.d.d100() <= breakChance)
	{
		// Success!
		game.message(WHITE_PAIR, "You break free from the web!", true);

		// Destroy the web that trapped the player
		if (trappingWeb) {
			trappingWeb->destroy();
			trappingWeb = nullptr;
		}

		webStuckTurns = 0;
		webStrength = 0;
		return true;
	}

	// Still stuck
	webStuckTurns--;
	if (webStuckTurns <= 0)
	{
		// Time expired, free anyway
		game.message(WHITE_PAIR, "You finally break free from the web!", true);

		// Destroy the web that trapped the player
		if (trappingWeb) {
			trappingWeb->destroy();
			trappingWeb = nullptr;
		}

		webStuckTurns = 0;
		webStrength = 0;
		return true;
	}

	// Message about remaining stuck
	game.message(WHITE_PAIR, "You're still stuck in the web. Turns remaining: " +
		std::to_string(webStuckTurns), true);
	return false;
}

// end of file: Player.cpp
