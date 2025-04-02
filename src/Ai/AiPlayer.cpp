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
#include "../Items.h"
#include "../ActorTypes/Gold.h"

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
	const Controls key = static_cast<Controls>(game.keyPress);
	Vector2D moveVector{ 0, 0 };

	// Handle confused state - randomly move or act
	if (owner.has_state(ActorState::IS_CONFUSED) && confusionTurns > 0)
	{
		// Decrease confusion duration
		confusionTurns--;

		// If confusion ended, inform the player
		if (confusionTurns == 0)
		{
			owner.remove_state(ActorState::IS_CONFUSED);
			game.message(WHITE_PAIR, "Your mind clears.", true);
		}
		else
		{
			// 50% chance of random movement
			if (game.d.d2() == 1)
			{
				// Random direction
				int randomDir = game.d.roll(0, 7);
				switch (randomDir)
				{
				case 0: moveVector = { -1, 0 }; break;  // North
				case 1: moveVector = { 1, 0 }; break;   // South
				case 2: moveVector = { 0, -1 }; break;  // West
				case 3: moveVector = { 0, 1 }; break;   // East
				case 4: moveVector = { -1, -1 }; break; // Northwest
				case 5: moveVector = { -1, 1 }; break;  // Northeast
				case 6: moveVector = { 1, -1 }; break;  // Southwest
				case 7: moveVector = { 1, 1 }; break;   // Southeast
				}

				game.message(CONFUSION_PAIR, "You stumble around in confusion!", true);
				game.gameStatus = Game::GameStatus::NEW_TURN;
			}
			else
			{
				// Process normal input but show a message
				game.message(CONFUSION_PAIR, "You struggle to control your movements...", true);

				// Check for movement keys as normal
				if (m.moves.find(key) != m.moves.end())
				{
					moveVector = m.moves.at(key);
					game.gameStatus = Game::GameStatus::NEW_TURN;
				}
				else
				{
					// Non-movement actions proceed as normal
					call_action(owner, key);
				}
			}
		}
	}
	else
	{
		// Normal movement handling (original code)
		if (m.moves.find(key) != m.moves.end())
		{
			moveVector = m.moves.at(key);
			game.gameStatus = Game::GameStatus::NEW_TURN;
		}
		else
		{
			call_action(owner, key);
		}
	}

	if (isWaiting) // when waiting tiles are triggered
	{
		isWaiting = false;
		game.map->tile_action(owner, game.map->get_tile_type(owner.position));
		look_on_floor(owner.position);
	}

	// if moving
	if (moveVector.x != 0 || moveVector.y != 0)
	{
		Vector2D targetPosition = owner.position + moveVector;
		game.map->tile_action(owner, game.map->get_tile_type(targetPosition));
		look_to_move(owner, targetPosition); // must check colissions before creature dies from attack
		look_to_attack(targetPosition, owner);
		look_on_floor(targetPosition);
		if (shouldComputeFOV)
		{
			shouldComputeFOV = false; // reset flag
			game.map->compute_fov();
		}
	}
}

void AiPlayer::load(const json& j)
{
	/*type is assigned in Ai::create()*/
}

void AiPlayer::save(json& j)
{
	j["type"] = static_cast<int>(AiType::PLAYER);
}

void AiPlayer::move(Creature& owner, Vector2D target)
{
	owner.position = target;
}

void AiPlayer::pick_item(Creature& owner)
{
	auto is_null = [](auto&& i) { return !i; };
	for (auto& i : game.container->inv)
	{
		if (i && i->position == owner.position)
		{
			// Special handling for gold
			if (i->actorData.name == "gold pile" && i->pickable)
			{
				auto goldPickable = dynamic_cast<Gold*>(i->pickable.get());
				if (goldPickable)
				{
					// Add gold to player
					owner.gold += goldPickable->amount;

					// Display message
					game.appendMessagePart(GOLD_PAIR, "You picked up ");
					game.appendMessagePart(GOLD_PAIR, std::to_string(goldPickable->amount));
					game.appendMessagePart(GOLD_PAIR, " gold.");
					game.finalizeMessage();

					// Remove gold pile from ground
					i.reset();
					std::erase_if(game.container->inv, is_null);
					return;
				}
			}

			// Normal item handling
			if (owner.container->add(std::move(i)))
			{
				std::erase_if(game.container->inv, is_null);
			}
			return;
		}
	}
}

void AiPlayer::drop_item(Creature& owner)
{
	owner.drop();
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
		for (const auto& item : owner.container->inv)
		{
			if (item != nullptr)
			{
				mvwprintw(inv, y, 1, "(%c) %s", shortcut, item->actorData.name.c_str());
				// if the actor is equipped, print a star
				if (item->has_state(ActorState::IS_EQUIPPED))
				{
					int nameLength = strlen(item->actorData.name.c_str()) + 5;
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
			/*game.gameStatus = Game::GameStatus::NEW_TURN;*/
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

void AiPlayer::look_on_floor(Vector2D target)
{
	// look for corpses or items
	for (const auto& i : game.container->inv)
	{
		if (i)
		{
			if (i->position == target)
			{
				game.appendMessagePart(WHITE_PAIR, std::format("There's a {} here\n", i->actorData.name));
				game.finalizeMessage();
			}
		}
	}
}

bool AiPlayer::look_to_attack(Vector2D& target, Creature& owner)
{
	// look for living actors to attack
	for (const auto& c : game.creatures)
	{
		if (c)
		{
			if (!c->destructible->is_dead() && c->position == target)
			{
				owner.attacker->attack(owner, *c);
				return false;
			}
		}
	}
	return true;
}


void AiPlayer::look_to_move(Creature& owner, const Vector2D& targetPosition)
{
	if (!game.map->is_collision(owner, game.map->get_tile_type(targetPosition), targetPosition))
	{
		move(owner, targetPosition);
		shouldComputeFOV = true;
	}
}


void AiPlayer::call_action(Creature& owner, Controls key)
{
	switch (key)
	{

	case Controls::WAIT:
	case Controls::WAIT_ARROW_NUMPAD:
	{
		game.gameStatus = Game::GameStatus::NEW_TURN;
		isWaiting = true;
		break;
	}

	case Controls::HIT_SELF:
	{
		game.player->attacker->attack(*game.player, *game.player);
		game.gameStatus = Game::GameStatus::NEW_TURN;
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
		game.gameStatus = Game::GameStatus::NEW_TURN;
		break;
	}

	case Controls::DROP:
	{
		drop_item(owner);
		game.gameStatus = Game::GameStatus::NEW_TURN;
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
		game.menus.push_back(std::make_unique<Menu>());
		break;
	}

	case Controls::DESCEND:
	{
		if (game.stairs->position == owner.position)
		{
			game.next_level(); // sets state to STARTUP
		}
		break;
	}

	case Controls::TARGET:
	{
		game.handle_ranged_attack();
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

	case Controls::OPEN_DOOR:
	{
		// Prompt for direction
		game.message(WHITE_PAIR, "Which direction? (use arrow keys or numpad)", true);
		int dirKey = getch();
		Vector2D doorPos = handle_direction_input(owner, dirKey);

		if (doorPos.x != 0 || doorPos.y != 0) { // Valid position
			if (game.map->is_door(doorPos)) {
				if (game.map->open_door(doorPos)) {
					game.message(WHITE_PAIR, "You open the door.", true);
					game.gameStatus = Game::GameStatus::NEW_TURN;
					// FOV is recalculated inside open_door method
				}
				else {
					game.message(WHITE_PAIR, "The door is already open.", true);
				}
			}
			else {
				game.message(WHITE_PAIR, "There is no door there.", true);
			}
		}
		else {
			game.message(WHITE_PAIR, "Invalid direction.", true);
		}
		break;
	}

	case Controls::CLOSE_DOOR:
	{
		// Prompt for direction
		game.message(WHITE_PAIR, "Which direction? (use arrow keys or numpad)", true);
		int dirKey = getch();
		Vector2D doorPos = handle_direction_input(owner, dirKey);

		if (doorPos.x != 0 || doorPos.y != 0) { // Valid position
			if (game.map->is_door(doorPos)) {
				if (game.map->close_door(doorPos)) {
					game.message(WHITE_PAIR, "You close the door.", true);
					game.gameStatus = Game::GameStatus::NEW_TURN;
					// FOV is recalculated inside close_door method
				}
				else {
					// Try to determine why door couldn't be closed
					if (game.map->get_actor(doorPos) != nullptr) {
						game.message(WHITE_PAIR, "Something is blocking the door.", true);
					}
					else {
						game.message(WHITE_PAIR, "The door is already closed.", true);
					}
				}
			}
			else {
				game.message(WHITE_PAIR, "There is no door there.", true);
			}
		}
		else {
			game.message(WHITE_PAIR, "Invalid direction.", true);
		}
		break;
	}

	default:
		break;
}
}

Vector2D AiPlayer::handle_direction_input(const Creature& owner, int dirKey)
{
	Vector2D delta{ 0, 0 };

	switch (dirKey) {
	case KEY_UP:
	case '8':
		delta = { -1, 0 }; // North
		break;
	case KEY_DOWN:
	case '2':
		delta = { 1, 0 }; // South
		break;
	case KEY_LEFT:
	case '4':
		delta = { 0, -1 }; // West
		break;
	case KEY_RIGHT:
	case '6':
		delta = { 0, 1 }; // East
		break;
	case '7':
		delta = { -1, -1 }; // Northwest
		break;
	case '9':
		delta = { -1, 1 }; // Northeast
		break;
	case '1':
		delta = { 1, -1 }; // Southwest
		break;
	case '3':
		delta = { 1, 1 }; // Southeast
		break;
	default:
		return { 0, 0 }; // Invalid direction
	}

	// Calculate the target position
	Vector2D targetPos = owner.position + delta;

	// Validate the position is within map bounds
	if (targetPos.x < 0 || targetPos.x >= game.map->get_width() ||
		targetPos.y < 0 || targetPos.y >= game.map->get_height()) {
		return { 0, 0 }; // Out of bounds
	}

	return targetPos;
}