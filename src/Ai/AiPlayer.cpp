// file: AiPlayer.cpp
#include <curses.h>
#include <libtcod.h>
#include <unordered_map>
#include <format>

#include "Ai.h"
#include "AiPlayer.h"
#include "../Game.h"
#include "../Menu/Menu.h"
#include "../Controls/Controls.h"
#include "../Actor/Actor.h"
#include "../Items.h"
#include "../ActorTypes/Gold.h"
#include "../Web.h"

//==INVENTORY==
constexpr int INVENTORY_HEIGHT = 29;
constexpr int INVENTORY_WIDTH = 60;

//==PLAYER_AI==
struct PossibleMoves
{
	std::unordered_map<Controls, Vector2D> moves = {
		{Controls::UP_ARROW, {-1, 0}},
		{Controls::UP_ARROW_NUMPAD, {-1, 0}},
		{Controls::DOWN_ARROW, {1, 0}},
		{Controls::DOWN_ARROW_NUMPAD, {1, 0}},
		{Controls::LEFT_ARROW, {0, -1}},
		{Controls::LEFT_ARROW_NUMPAD, {0, -1}},
		{Controls::RIGHT_ARROW, {0, 1}},
		{Controls::RIGHT_ARROW_NUMPAD, {0, 1}},
		{Controls::UP_LEFT_ARROW_NUMPAD, {-1, -1}},
		{Controls::UP_RIGHT_ARROW_NUMPAD, {-1, 1}},
		{Controls::DOWN_LEFT_ARROW_NUMPAD, {1, -1}},
		{Controls::DOWN_RIGHT_ARROW_NUMPAD, {1, 1}},
	};
} m;

void AiPlayer::update(Creature& owner)
{
	// If stuck in a web, try to break free and skip turn if still stuck
	if (game.player->isWebbed())
	{
		if (!game.player->tryBreakWeb())
		{
			// Still stuck, skip the player's turn
			game.gameStatus = Game::GameStatus::NEW_TURN;
			return;
		}
	}

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
	// Check if the inventory is already full
	if (owner.container && owner.container->invSize > 0 && owner.container->inv.size() >= owner.container->invSize) {
		game.message(WHITE_PAIR, "Your inventory is full! You can't carry any more items.", true);
		return;
	}

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

			auto name = i->actorData.name;

			// Normal item handling
			if (owner.container->add(std::move(i)))
			{
				std::erase_if(game.container->inv, is_null);

				// Sync ranged state after picking up an item
				owner.syncRangedState();

				game.message(WHITE_PAIR, std::format("You picked up the {}.", name), true);
			}
			// We don't need an else statement since Container::add() now handles the message
			return;
		}
	}
}

void AiPlayer::drop_item(Creature& owner)
{
	// If the inventory is empty, show a message and return
	if (owner.container->inv.empty()) {
		game.message(WHITE_PAIR, "Your inventory is empty!", true);
		return;
	}

	// Create a window for the drop item menu
	WINDOW* dropWin = newwin(INVENTORY_HEIGHT, INVENTORY_WIDTH, 0, 0);

	box(dropWin, 0, 0);
	wattron(dropWin, A_BOLD);
	mvwprintw(dropWin, 0, 1, "Select an item to drop");
	wattroff(dropWin, A_BOLD);

	// Display the inventory items
	int shortcut = 'a';
	int y = 1;

	for (const auto& item : owner.container->inv) {
		if (item) {
			// Display item with shortcut
			mvwprintw(dropWin, y, 1, "(%c) ", shortcut);

			// Add equipment marker if equipped
			if (item->has_state(ActorState::IS_EQUIPPED)) {
				wattron(dropWin, COLOR_PAIR(HPBARFULL_PAIR));
				wprintw(dropWin, "[E] ");
				wattroff(dropWin, COLOR_PAIR(HPBARFULL_PAIR));
			}

			// Display item name with color
			wattron(dropWin, COLOR_PAIR(item->actorData.color));
			wprintw(dropWin, "%s", item->actorData.name.c_str());
			wattroff(dropWin, COLOR_PAIR(item->actorData.color));

			y++;
			shortcut++;
		}
	}

	mvwprintw(dropWin, y + 1, 1, "Press a letter to drop an item, or ESC to cancel");
	wrefresh(dropWin);

	// Wait for player input
	int input = getch();

	// Process the input
	if (input == static_cast<int>(Controls::ESCAPE)) {
		// Cancel dropping
		game.message(WHITE_PAIR, "Drop canceled.", true);
	}
	else if (input >= 'a' && input < 'a' + static_cast<int>(owner.container->inv.size())) {
		// Valid item selection
		int index = input - 'a';

		if (index >= 0 && index < static_cast<int>(owner.container->inv.size())) {
			// Get the selected item
			Item* itemToDrop = owner.container->inv[index].get();
			if (itemToDrop) {
				// Display the item name before dropping
				std::string itemName = itemToDrop->actorData.name;

				// Drop the selected item
				owner.drop(*itemToDrop);

				// Show dropped message with the item name
				game.message(WHITE_PAIR, "You dropped the " + itemName + ".", true);

				// Set game status to register the turn
				game.gameStatus = Game::GameStatus::NEW_TURN;
			}
		}
	}

	// Clean up
	delwin(dropWin);
	clear();
	refresh();
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
				// Display basic item info
				mvwprintw(inv, y, 1, "(%c) ", shortcut);

				// Add colors and status indicators
				if (item->has_state(ActorState::IS_EQUIPPED))
				{
					wattron(inv, COLOR_PAIR(HPBARFULL_PAIR)); // Green for equipped items
					wprintw(inv, "[E] ");
					wattroff(inv, COLOR_PAIR(HPBARFULL_PAIR));
				}

				// Show the item name with its color
				wattron(inv, COLOR_PAIR(item->actorData.color));
				wprintw(inv, "%s", item->actorData.name.c_str());
				wattroff(inv, COLOR_PAIR(item->actorData.color));

				// Show item information
				if (item->value > 0)
				{
					wattron(inv, COLOR_PAIR(GOLD_PAIR)); // Gold color for value
					wprintw(inv, " (%d gp)", item->value);
					wattroff(inv, COLOR_PAIR(GOLD_PAIR));
				}

				// For weapons, show damage dice
				if (auto weapon = item->pickable.get(); weapon &&
					(dynamic_cast<Dagger*>(weapon) ||
						dynamic_cast<LongSword*>(weapon) ||
						dynamic_cast<ShortSword*>(weapon) ||
						dynamic_cast<Longbow*>(weapon) ||
						dynamic_cast<Staff*>(weapon)))
				{
					// Get the roll string - this requires some casting
					std::string roll;
					if (auto* dagger = dynamic_cast<Dagger*>(weapon))
						roll = dagger->roll;
					else if (auto* sword = dynamic_cast<LongSword*>(weapon))
						roll = sword->roll;
					else if (auto* ssword = dynamic_cast<ShortSword*>(weapon))
						roll = ssword->roll;
					else if (auto* bow = dynamic_cast<Longbow*>(weapon))
						roll = bow->roll;
					else if (auto* staff = dynamic_cast<Staff*>(weapon))
						roll = staff->roll;

					if (!roll.empty())
					{
						wattron(inv, COLOR_PAIR(WHITE_PAIR));
						wprintw(inv, " [%s dmg]", roll.c_str());
						wattroff(inv, COLOR_PAIR(WHITE_PAIR));
					}
				}

				// Check if it's a ranged weapon
				if (auto weapon = item->pickable.get(); weapon)
				{
					if (auto* bow = dynamic_cast<Longbow*>(weapon))
					{
						wattron(inv, COLOR_PAIR(LIGHTNING_PAIR));
						wprintw(inv, " [Rng]");
						wattroff(inv, COLOR_PAIR(LIGHTNING_PAIR));
					}
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
	wattron(inv, A_BOLD);
	if (owner.container->invSize > 0) {
		mvwprintw(inv, 0, 1, "Inventory (%zu/%zu)", owner.container->inv.size(), owner.container->invSize);
	}
	else {
		mvwprintw(inv, 0, 1, "Inventory");
	}
	wattroff(inv, A_BOLD);

	try
	{
		if (owner.container->inv.size() > 0)
		{
			display_inventory_items(inv, owner);

			// Display controls at the bottom
			int y = INVENTORY_HEIGHT - 2;
			mvwprintw(inv, y, 1, "Press a-z to use an item");
			mvwprintw(inv, y + 1, 1, "ESC to cancel");
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
		refresh();
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
		refresh();
	}
	else
	{
		delwin(inv);
		clear();
		refresh();
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
			Item* item = owner.container->inv.at(index).get();

			// Sync ranged state since we might use/equip/unequip an item
			owner.syncRangedState();

			return item;
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
		// Check if there's a web at the target position
		bool webEffect = false;

		// Search for a web at the target position
		for (const auto& obj : game.objects)
		{
			if (obj && obj->position == targetPosition &&
				obj->actorData.name == "spider web")
			{
				// Found a web, cast to proper type
				Web* web = dynamic_cast<Web*>(obj.get());
				if (web)
				{
					// Apply the web effect - returns true if player gets caught
					webEffect = web->applyEffect(owner);
					break;
				}
			}
		}

		// If the player wasn't caught in a web, proceed with movement
		if (!webEffect)
		{
			move(owner, targetPosition);
			shouldComputeFOV = true;
		}
	}
}


void AiPlayer::call_action(Creature& owner, Controls key)
{
	switch (key)
	{

	/*case Controls::WAIT:*/
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
		// add xp to player
		game.player->destructible->xp += 1000;
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

		if (doorPos.x != 0 || doorPos.y != 0)
		{ // Valid position
			if (game.map->is_door(doorPos))
			{
				if (game.map->open_door(doorPos))
				{
					game.message(WHITE_PAIR, "You open the door.", true);
					game.gameStatus = Game::GameStatus::NEW_TURN;
					// FOV is recalculated inside open_door method
				}
				else
				{
					game.message(WHITE_PAIR, "The door is already open.", true);
				}
			}
			else
			{
				game.message(WHITE_PAIR, "There is no door there.", true);
			}
		}
		else
		{
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

		if (doorPos.x != 0 || doorPos.y != 0)
		{ // Valid position
			if (game.map->is_door(doorPos))
			{
				if (game.map->close_door(doorPos))
				{
					game.message(WHITE_PAIR, "You close the door.", true);
					game.gameStatus = Game::GameStatus::NEW_TURN;
					// FOV is recalculated inside close_door method
				}
				else
				{
					// Try to determine why door couldn't be closed
					if (game.map->get_actor(doorPos) != nullptr)
					{
						game.message(WHITE_PAIR, "Something is blocking the door.", true);
					}
					else
					{
						game.message(WHITE_PAIR, "The door is already closed.", true);
					}
				}
			}
			else
			{
				game.message(WHITE_PAIR, "There is no door there.", true);
			}
		}
		else
		{
			game.message(WHITE_PAIR, "Invalid direction.", true);
		}
		break;
	}

	case Controls::REST:
	{
		game.player->rest();
		break;
	}

	case Controls::HELP:
	{
		game.display_help();
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