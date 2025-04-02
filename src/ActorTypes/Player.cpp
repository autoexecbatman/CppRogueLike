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

ActorData playerData{ '@', "Player", WHITE_PAIR };

Player::Player(Vector2D position, int maxHp, int dr, std::string corpseName, int xp, int thaco, int armorClass)
	:
	Creature(position, playerData)
{
	add_state(ActorState::CAN_SWIM);

	auto roll3d6 = []() { return game.d.d6() + game.d.d6() + game.d.d6(); };
	strength = roll3d6();
	dexterity = roll3d6();
	constitution = roll3d6();
	intelligence = roll3d6();
	wisdom = roll3d6();
	charisma = roll3d6();

	gold = 100;

	attacker = std::make_unique<Attacker>("D2");
	destructible = std::make_unique<PlayerDestructible>(maxHp, dr, corpseName, xp, thaco, armorClass);
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

	// If player is confused, add visual indicators around them
	if (has_state(ActorState::IS_CONFUSED))
	{
		// Add swirling indicators around the player in confusion color
		attron(COLOR_PAIR(CONFUSION_PAIR));

		// Top positions
		if (position.y > 0) {
			mvaddch(position.y - 1, position.x, '~');
			if (position.x > 0)
				mvaddch(position.y - 1, position.x - 1, '?');
			if (position.x < MAP_WIDTH - 1)
				mvaddch(position.y - 1, position.x + 1, '?');
		}

		// Side positions
		if (position.x > 0)
			mvaddch(position.y, position.x - 1, '~');
		if (position.x < MAP_WIDTH - 1)
			mvaddch(position.y, position.x + 1, '~');

		// Bottom positions
		if (position.y < MAP_HEIGHT - 1) {
			mvaddch(position.y + 1, position.x, '~');
			if (position.x > 0)
				mvaddch(position.y + 1, position.x - 1, '?');
			if (position.x < MAP_WIDTH - 1)
				mvaddch(position.y + 1, position.x + 1, '?');
		}

		attroff(COLOR_PAIR(CONFUSION_PAIR));
	}
}

// end of file: Player.cpp
