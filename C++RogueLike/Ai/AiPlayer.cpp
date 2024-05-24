// file: AiPlayer.cpp

#include <unordered_map>
#include <format>
#include <gsl/util>

#include "AiPlayer.h"
#include "../Game.h"
#include "../Menu/Menu.h"
#include "../Controls/Controls.h"

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

void AiPlayer::update(Actor& owner)
{
	// if owner is not the player, return
	if (&owner != game.player)
	{
		game.log("AiPlayer::update(Actor& owner) owner.ai != game.player");
		return;
	}

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

int AiPlayer::get_next_level_xp(Actor& owner)
{
	constexpr int LEVEL_UP_BASE = 200;
	constexpr int LEVEL_UP_FACTOR = 150;

	return LEVEL_UP_BASE + (owner.playerLevel * LEVEL_UP_FACTOR);
}

// returns true if the player is dead
void AiPlayer::levelup_update(Actor& owner)
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

void AiPlayer::pick_item(Actor& owner)
{
	if (game.actors.empty())
	{
		game.log("Error: PlayerAi::pick_item(Actor& owner). game.actors is empty");
		exit(-1);
	}

	bool found = false; // true if an item was found at the player's position

	// search for an item at the player's position
	for (auto& actor : game.actors) // we don't use a reference here because we are modifying the vector
	{
		// Skip null actors
		if (actor == nullptr)
		{
			game.log("Error: PlayerAi::pick_item(Actor& owner). game.actors contains null pointer");
			exit(-1);
		}

		game.log("Checking actor " + actor->actorData.name);

		// Skip actors without a pickable component
		if (actor->pickable == nullptr)
		{
			game.log("Skipping actor " + actor->actorData.name + " because it has no pickable component");
			continue;
		}

		// Check if the actor can be picked up at the player's position
		if (is_pickable_at_position(*actor, owner))
		{
			// Try to pick up the actor
			found = try_pick_actor(std::move(actor), owner); // the actor gets deleted in this function if it was picked up

			if (found)
			{
				break;
			}
			else
			{
				game.message(WHITE_PAIR, "There is nothing to pick up.", true);
			}
		}
	}

	game.gameStatus = Game::GameStatus::NEW_TURN;
}

void AiPlayer::drop_item(Actor& owner)
{
	Actor* actorPtr = chose_from_inventory(owner, 'a');
	if (actorPtr)
	{
		auto it = std::find_if(owner.container->inventoryList.begin(), owner.container->inventoryList.end(), [&actorPtr](const auto& item) { return item.get() == actorPtr; });
		std::unique_ptr<Actor> actor = std::move(*it);
		if (actor)
		{
			actor->pickable->drop(std::move(actor), owner);
			game.gameStatus = Game::GameStatus::NEW_TURN;
		}
	}
}

bool AiPlayer::is_pickable_at_position(const Actor& actor, const Actor& owner) const
{
	return actor.position.x == owner.position.x && actor.position.y == owner.position.y;
}

bool AiPlayer::try_pick_actor(std::unique_ptr<Actor> actor, Actor& owner)
{
	game.log("Trying to pick actor " + actor->actorData.name);
	bool picked{};
	std::string actorName = actor->actorData.name;

	// actor is destroyed if picked what can we do about it ? 
	// we can't use actor anymore
	try
	{
		picked = actor->pickable->pick(std::move(actor), owner);
	}
	catch (const std::exception& e)
	{
		game.log("Error: AiPlayer::try_pick_actor(Actor& actor, Actor& owner). " + std::string(e.what()));
		picked = false;
	}

	game.log("AiPlayer::try_pick_actor(Actor& actor, Actor& owner) picked = " + std::to_string(picked));
	game.log("AiPlayer::try_pick_actor(Actor& actor, Actor& owner) actor.name = " + actorName);

	if (picked)
	{
		game.message(WHITE_PAIR, std::format("You take the {}.", actorName), true);
	}
	else
	{
		game.message(HPBARMISSING_PAIR, "Your inventory is full.",true);
	}

	return picked;
}

void AiPlayer::display_inventory_items(WINDOW* inv, const Actor& owner) noexcept
{
	int shortcut = 'a';
	int y = 1;
	try
	{
		for (const auto& actor : owner.container->inventoryList)
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

void AiPlayer::display_inventory(Actor& owner)
{
	refresh();

	WINDOW* inv = newwin(INVENTORY_HEIGHT, INVENTORY_WIDTH, 0, 0);

	box(inv, 0, 0);
	mvwprintw(inv, 0, 0, "Inventory");

	try
	{
		if (owner.container->inventoryList.size() > 0)
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

Actor* AiPlayer::chose_from_inventory(Actor& owner, int ascii)
{
	game.log("You chose from inventory");
	if (owner.container != nullptr)
	{
		const size_t index = ascii - 'a';
		if (index >= 0 && index < owner.container->inventoryList.size())
		{
			return owner.container->inventoryList.at(index).get();
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
bool AiPlayer::move_or_attack(Actor& owner, Vector2D target)
{
	game.log("Player tries to move or attack");

	if (game.map->is_wall(target))
	{
		return false;
	}
	else if (game.map->is_water(target))
	{
		if (!owner.flags.canSwim)
		{
			return false;
		}
		else
		{
			// print message that player is swimming
			mvprintw(0, 0, "You are swimming!");
			refresh();
			mvprintw(1, 0, "Press any key to continue.");
			getch();
			clear();

			owner.position = target;
			return true;
		}
	}

	// look for living actors to attack
	// TODO : should we iterate over the entire list ?
	// in order to check that there is nothing to attack and tile is empty ?
	for (const auto& actor : game.actors)
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

	// look for corpses or items
	for (const auto& actor : game.actors)
	{
		if ((actor->destructible->is_dead() || actor->pickable) && actor->position == target)
		{
			game.appendMessagePart(WHITE_PAIR, std::format("There's a {} here\n", actor->actorData.name));
			game.finalizeMessage();
		}
	}

	owner.position = target;

	return true;
}

void AiPlayer::call_action(Actor& owner, Controls key)
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
