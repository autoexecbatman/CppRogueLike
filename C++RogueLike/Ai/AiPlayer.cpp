// file: AiPlayer.cpp
#include <curses.h>
#include <libtcod.h>
#include <unordered_map>
#include <format>
#include <gsl/util>

#include "Ai.h"
#include "AiPlayer.h"
#include "../Game.h"
#include "../Menu/Menu.h"
#include "../Controls/Controls.h"
#include "../Actor/Actor.h"

//==INVENTORY==
constexpr int INVENTORY_HEIGHT = 29;
constexpr int INVENTORY_WIDTH = 30;

//==PLAYER_AI==
struct PossibleMoves
{
	std::unordered_map<Controls, Vector2D> moves = {
		{Controls::UP, {-1, 0}},
		{Controls::DOWN, {1, 0}},
		{Controls::LEFT, {0, -1}},
		{Controls::RIGHT, {0, 1}},
		{Controls::UP_LEFT, {-1, -1}},
		{Controls::UP_RIGHT, {-1, 1}},
		{Controls::DOWN_LEFT, {1, -1}},
		{Controls::DOWN_RIGHT, {1, 1}}
	};
} m;

void AiPlayer::update(Creature& owner)
{
	game.log("AiPlayer::update(Actor& owner)");
	levelup_update(owner); // level up if needed

	const Controls key = static_cast<Controls>(game.keyPress);
	Vector2D moveVector{ 0, 0 };

	// Check for movement first
	if (m.moves.find(key) != m.moves.end())
	{
		moveVector = m.moves.at(key);
	}
	else
	{
		call_action(owner, key);
	}

	// compute FOV if needed (if player moved)
	if (moveVector.x != 0 || moveVector.y != 0)
	{
		game.gameStatus = Game::GameStatus::NEW_TURN;
		Vector2D targetPosition = owner.position + moveVector;
		if (move_or_attack(owner, targetPosition))
		{
			game.log("AiPlayer::update(Actor& owner) moveOrAttack(owner, owner.posX + dx, owner.posY + dy)");
			game.map->compute_fov();
		}
	}
	std::clog << "PlayerAi::update(Actor& owner) end" << std::endl;
}

void AiPlayer::load(TCODZip& zip)
{
	// this is a player, so nothing to load
	// because the player is always the same ? 
	// or is this for loading a saved game ?
	// if so, then the player is not always the same
	// so this is not correct
	// but I don't know how to do it yet
}

void AiPlayer::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<AiType>>(AiType::PLAYER));
}

int AiPlayer::get_next_level_xp(Creature& owner)
{
	constexpr int LEVEL_UP_BASE = 200;
	constexpr int LEVEL_UP_FACTOR = 150;

	return LEVEL_UP_BASE + (owner.playerLevel * LEVEL_UP_FACTOR);
}

void AiPlayer::levelup_update(Creature& owner)
{
	game.log("AiPlayer::levelUpUpdate(Actor& owner)");
	// level up if needed
	int levelUpXp = get_next_level_xp(owner);

	if (owner.destructible->xp >= levelUpXp)
	{
		game.player->playerLevel++;
		owner.destructible->xp -= levelUpXp;
		game.gui->log_message(WHITE_PAIR, "Your battle skills grow stronger! You reached level %d", game.player->playerLevel);
		game.player->calculate_thaco();
		game.dispay_levelup(game.player->playerLevel);
	}
}

void AiPlayer::pick_item(Creature& owner)
{
	owner.pick();
	game.gameStatus = Game::GameStatus::NEW_TURN;
}

void AiPlayer::drop_item(Creature& owner)
{
	owner.drop();
	game.gameStatus = Game::GameStatus::NEW_TURN;
}

bool AiPlayer::is_pickable_at_position(const Actor& actor, const Actor& owner) const
{
	return actor.position == owner.position;
}

void AiPlayer::display_inventory_items(WINDOW* inv, const Creature& owner) noexcept
{
	int shortcut = 'a';
	int y = 1;
	try
	{
		for (const auto& actor : owner.container->inv)
		{
			if (actor != nullptr)
			{
				mvwprintw(inv, y, 1, "(%c) %s", shortcut, actor->actorData.name.c_str());
				// if the actor is equipped, print a star
				if (actor->flags.isEquipped)
				{
					int nameLength = strlen(actor->actorData.name.c_str()) + 5;
					mvwprintw(inv, y, nameLength, "*");
				}
			}
			y++;
			shortcut++;
		}
	}
	catch (const std::exception& e)
	{
		std::cout << "Error: PlayerAi::displayInventoryItems(WINDOW* inv, const Actor& owner). " << e.what() << std::endl;
		exit(-1);
	}
}

void AiPlayer::display_inventory(Creature& owner)
{
	refresh();

	WINDOW* inv = newwin(INVENTORY_HEIGHT, INVENTORY_WIDTH, 0, 0);

	box(inv, 0, 0);
	mvwprintw(inv, 0, 0, "Inventory");

	try
	{
		if (owner.container->inv.size() > 0)
		{
			display_inventory_items(inv, owner);
		}
		else
		{
			const int y = 1;
			mvwprintw(inv, y, 1, "Your inventory is empty.");
		}
	}
	catch (const std::exception& e)
	{
		std::cout << "Error: PlayerAi::display_inventory(Actor& owner). " << e.what() << std::endl;
		exit(-1);
	}

	wrefresh(inv);

	const int inventoryInput = getch();
	if (inventoryInput == static_cast<int>(Controls::ESCAPE))
	{
		delwin(inv);
		clear();
	}
	else if (inventoryInput >= 'a' && inventoryInput <= 'z')
	{
		const auto& actor = chose_from_inventory(owner, inventoryInput);
		if (actor)
		{
			actor->pickable->use(*actor, owner);
			game.gameStatus = Game::GameStatus::NEW_TURN;
		}
		delwin(inv);
		clear();
	}
	else
	{
		delwin(inv);
		clear();
	}
}

Item* AiPlayer::chose_from_inventory(Creature& owner, int ascii)
{
	game.log("You chose from inventory");
	if (owner.container != nullptr)
	{
		const size_t index = ascii - 'a';
		if (index >= 0 && index < owner.container->inv.size())
		{
			return owner.container->inv.at(index).get();
		}
		else
		{
			// if the index is out of bounds
			return nullptr;
		}
	}
	else
	{
		game.log("Error: choseFromInventory() called on actor with no container.");
		exit(EXIT_FAILURE);
	}
}

// returns true if the action was successful
bool AiPlayer::move_or_attack(Creature& owner, Vector2D target)
{
	game.log("Player tries to move or attack");

	// check tile state
	// and act accordingly
	game.map->tile_action(game.map->get_tile_t(target));

	auto move_player = [&owner, &target] { owner.position = target; return true; };
	auto is_wall = [target]{ return !game.map->is_wall(target); };

	auto print_message = [] {
		mvprintw(0, 0, "You are swimming!");
		refresh();
		mvprintw(1, 0, "Press any key to continue.");
		getch();
		clear();
	};
	auto check_water = [&owner, &target] { return !(game.map->is_water(target) && !owner.flags.canSwim); };

	if (game.map->is_wall(target))
	{
		return false;
	}

	check_water();

	auto not_nullptr = [](const auto& actor) { return actor != nullptr; };
	auto is_dead = [](const auto& actor) { return actor->destructible->is_dead(); };
	auto is_position = [&target](const auto& actor) { return actor->position == target; };
	auto attack_and_return_false = [&owner](auto&& actor) { owner.attacker->attack(owner, *actor); return false; };
	auto return_false = [] { return false; };
	auto append_message = [](const auto& actor) { game.appendMessagePart(WHITE_PAIR, std::format("There's a {} here\n", actor->actorData.name)); game.finalizeMessage(); };

	// look for living actors to attack
	// TODO : should we iterate over the entire list ?
	// in order to check that there is nothing to attack and tile is empty ?
	for (const auto& actor : game.creatures)
	{
		if (actor != nullptr)
		{
			if (!actor->destructible->is_dead() && actor->position == target)
			{
				owner.attacker->attack(owner, *actor);
				return false;
			}
		}
		else
		{
			std::cout << "Error: moveOrAttack() called on actor with null actor." << std::endl;
			exit(-1);
		}
	}

	std::ranges::for_each(game.creatures
		| std::views::filter(not_nullptr)
		| std::views::filter(is_dead)
		| std::views::filter(is_position),
		attack_and_return_false
	);

	// look for corpses or items
	for (const auto& actor : game.creatures)
	{
		if (actor->destructible->is_dead() && actor->position == target)
		{
			game.appendMessagePart(WHITE_PAIR, std::format("There's a {} here\n", actor->actorData.name));
			game.finalizeMessage();
		}
	}

	std::ranges::for_each(game.creatures
		| std::views::filter(not_nullptr)
		| std::views::filter(is_dead)
		| std::views::filter(is_position),
		append_message
	);

	for (const auto& actor : game.container->inv)
	{
		if (actor->position == target)
		{
			game.appendMessagePart(WHITE_PAIR, std::format("There's a {} here\n", actor->actorData.name));
			game.finalizeMessage();
		}
	}

	std::ranges::for_each(game.container->inv
			| std::views::filter(is_position),
				append_message
		);

	owner.position = target; // move player

	return true;
}

void AiPlayer::call_action(Creature& owner, Controls key)
{
	switch (key)
	{
	case Controls::WAIT:
	case Controls::WAIT_ARROW_NUMPAD:
	{
		game.gameStatus = Game::GameStatus::NEW_TURN;
		break;
	}

	case Controls::HIT_SELF:
	{
		game.player->attacker->attack(*game.player, *game.player);
		break;
	}

	case Controls::MOUSE:
	{
		std::cout << "mouse" << std::endl;
		request_mouse_pos();
		break;
	}

	case Controls::PICK:
	case Controls::PICK_SHIFT_STAR:
	case Controls::PICK_NUMPAD:
	{
		pick_item(owner);
		break;
	}

	case Controls::DROP:
	{
		drop_item(owner);
		break;
	}

	case Controls::INVENTORY:
	{
		display_inventory(owner);
		break;
	}

	case Controls::QUIT:
	{
		game.run = false;
		game.message(WHITE_PAIR, "You quit the game ! Press any key ...", true);
		break;
	}

	case Controls::ESCAPE: // if escape key is pressed bring the game menu
	{
		Menu menu;
		menu.menu();
		break;
	}

	case Controls::DESCEND:
	{
		if (game.stairs->position == owner.position)
		{
			game.next_level();
		}
		break;
	}

	case Controls::TARGET:
	{
		game.target();
		break;
	}

	case Controls::CHAR_SHEET:
	{
		game.display_character_sheet();
		break;
	}

	case Controls::DEBUG:
	{
		game.display_debug_messages();
		break;
	}

	case Controls::REVEAL:
	{
		game.map->reveal();
		break;
	}

	case Controls::REGEN:
	{
		game.map->regenerate();
		break;
	}

	default:
		break;
}
}
