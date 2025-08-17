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
#include "../Items/Items.h"
#include "../ActorTypes/Gold.h"
#include "../Objects/Web.h"
#include "../Factories/ItemCreator.h"
#include "../ActorTypes/Monsters.h"
#include "../ActorTypes/Player.h"
#include "../Systems/LevelUpSystem.h"
#include "../UI/InventoryUI.h"

//==INVENTORY==
constexpr int INVENTORY_HEIGHT = 29;
constexpr int INVENTORY_WIDTH = 60;

//==PLAYER_AI==
struct PossibleMoves
{
	std::unordered_map<Controls, Vector2D> moves = {
		// Arrow keys
		{Controls::UP_ARROW, {-1, 0}},
		{Controls::DOWN_ARROW, {1, 0}},
		{Controls::LEFT_ARROW, {0, -1}},
		{Controls::RIGHT_ARROW, {0, 1}},
    
		// WASD movement
		{Controls::W_KEY, {-1, 0}},
		{Controls::S_KEY, {1, 0}},
		{Controls::A_KEY, {0, -1}},
		{Controls::D_KEY, {0, 1}},
    
		// WASD diagonals
		{Controls::Q_KEY, {-1, -1}},    // up-left
		{Controls::E_KEY, {-1, 1}},     // up-right
		{Controls::Z_KEY, {1, -1}},     // down-left  
		{Controls::C_KEY, {1, 1}},      // down-right
	};
} m;

void AiPlayer::update(Creature& owner)
{
	// If stuck in a web, try to break free and skip turn if still stuck
	if (game.player->is_webbed())
	{
		if (!game.player->try_break_web())
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
			game.message(WHITE_BLACK_PAIR, "Your mind clears.", true);
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

				game.message(WHITE_GREEN_PAIR, "You stumble around in confusion!", true);
				game.gameStatus = Game::GameStatus::NEW_TURN;
			}
			else
			{
				// Process normal input but show a message
				game.message(WHITE_GREEN_PAIR, "You struggle to control your movements...", true);

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
		// Only call tile_action after successful move (inside look_to_move)
		look_to_move(owner, targetPosition); // must check collisions before creature dies from attack
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
	// Check if the inventory is already full (including equipped items)
	auto* player = dynamic_cast<Player*>(&owner);
	size_t totalItems = owner.container->inv.size();
	if (player) {
		totalItems += player->equippedItems.size(); // Count equipped items too
	}
	
	if (owner.container && owner.container->invSize > 0 && totalItems >= owner.container->invSize) {
		game.message(WHITE_BLACK_PAIR, "Your inventory is full! You can't carry any more items.", true);
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
					game.appendMessagePart(YELLOW_BLACK_PAIR, "You picked up ");
					game.appendMessagePart(YELLOW_BLACK_PAIR, std::to_string(goldPickable->amount));
					game.appendMessagePart(YELLOW_BLACK_PAIR, " gold.");
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

				game.message(WHITE_BLACK_PAIR, std::format("You picked up the {}.", name), true);
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
		game.message(WHITE_BLACK_PAIR, "Your inventory is empty!", true);
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

	// First, show equipped items
	auto* player = dynamic_cast<Player*>(&owner);
	if (player && !player->equippedItems.empty())
	{
		mvwprintw(dropWin, y++, 1, "=== EQUIPPED ===");
		
		for (const auto& equipped : player->equippedItems)
		{
			if (equipped.item)
			{
				mvwprintw(dropWin, y, 1, "(%c) ", shortcut);
				
				// Show equipped marker
				wattron(dropWin, COLOR_PAIR(WHITE_GREEN_PAIR));
				wprintw(dropWin, "[E] ");
				wattroff(dropWin, COLOR_PAIR(WHITE_GREEN_PAIR));
				
				// Show slot type
				std::string slotName = (equipped.slot == EquipmentSlot::RIGHT_HAND) ? "Right: " : 
									   (equipped.slot == EquipmentSlot::LEFT_HAND) ? "Left: " : 
									   (equipped.slot == EquipmentSlot::BODY) ? "Body: " : "Other: ";
				wprintw(dropWin, "%s", slotName.c_str());
				
				// Display item name with color
				wattron(dropWin, COLOR_PAIR(equipped.item->actorData.color));
				wprintw(dropWin, "%s", equipped.item->actorData.name.c_str());
				wattroff(dropWin, COLOR_PAIR(equipped.item->actorData.color));
				
				y++;
				shortcut++;
			}
		}
		
		if (!owner.container->inv.empty())
		{
			mvwprintw(dropWin, y++, 1, "=== INVENTORY ===");
		}
	}

	// Then show regular inventory items
	for (const auto& item : owner.container->inv)
	{
		if (item)
		{
			// Display item with shortcut
			mvwprintw(dropWin, y, 1, "(%c) ", shortcut);

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
		game.message(WHITE_BLACK_PAIR, "Drop canceled.", true);
	}
	else if (input >= 'a' && input < 'a' + static_cast<int>((player ? player->equippedItems.size() : 0) + owner.container->inv.size())) {
		// Valid item selection
		int index = input - 'a';
		
		// Check if selecting equipped item
		if (player && index < static_cast<int>(player->equippedItems.size()))
		{
			// Dropping equipped item - unequip it first
			const auto& equipped = player->equippedItems[index];
			std::string itemName = equipped.item->actorData.name;
			
			// Unequip the item (this returns it to inventory)
			EquipmentSlot slot = equipped.slot;
			player->unequip_item(slot);
			
			// Find the item in inventory and drop it
			for (auto& item : owner.container->inv)
			{
				if (item && item->actorData.name == itemName)
				{
					owner.drop(*item);
					break;
				}
			}
			
			game.message(WHITE_BLACK_PAIR, "You unequipped and dropped the " + itemName + ".", true);
		}
		else
		{
			// Dropping regular inventory item
			int inventoryIndex = index - (player ? static_cast<int>(player->equippedItems.size()) : 0);
			
			if (inventoryIndex >= 0 && inventoryIndex < static_cast<int>(owner.container->inv.size())) {
				// Get the selected item
				Item* itemToDrop = owner.container->inv[inventoryIndex].get();
				if (itemToDrop) {
					// Display the item name before dropping
					std::string itemName = itemToDrop->actorData.name;

					// Drop the selected item
					owner.drop(*itemToDrop);

					// Show dropped message with the item name
					game.message(WHITE_BLACK_PAIR, "You dropped the " + itemName + ".", true);
				}
			}
		}
		
		// Set game status to register the turn
		game.gameStatus = Game::GameStatus::NEW_TURN;
	}

	// Clean up
	delwin(dropWin);
	
	// CRITICAL FIX: Restore the game display
	clear();
	refresh();
	game.restore_game_display();
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
					wattron(inv, COLOR_PAIR(WHITE_GREEN_PAIR)); // Green for equipped items
					wprintw(inv, "[E] ");
					wattroff(inv, COLOR_PAIR(WHITE_GREEN_PAIR));
				}

				// Show the item name with its color
				wattron(inv, COLOR_PAIR(item->actorData.color));
				wprintw(inv, "%s", item->actorData.name.c_str());
				wattroff(inv, COLOR_PAIR(item->actorData.color));

				// Show item information
				if (item->value > 0)
				{
					wattron(inv, COLOR_PAIR(YELLOW_BLACK_PAIR)); // Gold color for value
					wprintw(inv, " (%d gp)", item->value);
					wattroff(inv, COLOR_PAIR(YELLOW_BLACK_PAIR));
				}

				// For weapons, show damage dice
				if (auto* weapon = dynamic_cast<Weapon*>(item->pickable.get()))
				{
					std::string damageInfo = " [" + weapon->roll + (weapon->is_ranged() ? " rng dmg]" : " dmg]");
					wattron(inv, COLOR_PAIR(WHITE_BLACK_PAIR));
					wprintw(inv, "%s", damageInfo.c_str());
					wattroff(inv, COLOR_PAIR(WHITE_BLACK_PAIR));
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
	InventoryUI inventoryUI;
	inventoryUI.display(owner);
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
				game.appendMessagePart(WHITE_BLACK_PAIR, std::format("There's a {} here\n", i->actorData.name));
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
				// Handle multiple attacks for fighters
				auto playerPtr = dynamic_cast<Player*>(&owner);
				if (playerPtr)
				{
					// Increment round counter for attack pattern tracking
					playerPtr->roundCounter++;
					
					// Calculate number of attacks this round
					int attacksThisRound = 1; // Default single attack
					
					if (playerPtr->attacksPerRound >= 2.0f)
					{
						// 2 attacks per round
						attacksThisRound = 2;
					}
					else if (playerPtr->attacksPerRound >= 1.5f)
					{
						// 3/2 attacks per round (alternating pattern: 2, 1, 2, 1...)
						attacksThisRound = (playerPtr->roundCounter % 2 == 1) ? 2 : 1;
					}
					
					// Perform the attacks
					for (int i = 0; i < attacksThisRound; i++)
					{
						if (c->destructible && !c->destructible->is_dead()) // Check if target is still alive
						{
							if (i > 0)
							{
								// Display follow-up attack message
								game.appendMessagePart(WHITE_BLACK_PAIR, "Follow-up attack: ");
								game.finalizeMessage();
							}
							// Use dual wield attack for players
							owner.attacker->attack_with_dual_wield(owner, *c);
						}
						else
						{
							// Target died, no more attacks needed
							break;
						}
					}
					
					// Clean up dead creatures after combat
					game.cleanup_dead_creatures();
				}
				else
				{
				// Non-player creatures get single attack
				owner.attacker->attack(owner, *c);
				}
				
				// Clean up dead creatures after combat
				game.cleanup_dead_creatures();
				return false;
			}
		}
	}
	return true;
}


void AiPlayer::look_to_move(Creature& owner, const Vector2D& targetPosition)
{
	TileType targetTileType = game.map->get_tile_type(targetPosition);
	
	if (!game.map->is_collision(owner, targetTileType, targetPosition))
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
			// Call tile_action after successful move
			game.map->tile_action(owner, targetTileType);
			shouldComputeFOV = true;
		}
	}
	else
	{
		// Handle collision messages
		switch (targetTileType)
		{
		case TileType::WATER:
			if (!owner.has_state(ActorState::CAN_SWIM))
			{
				game.log("You can't swim.");
				game.message(WHITE_BLACK_PAIR, "You can't swim.", true);
			}
			break;
		case TileType::WALL:
			// Wall collision already handled elsewhere
			break;
		case TileType::CLOSED_DOOR:
			// Door collision - could add message here if desired
			break;
		default:
			break;
		}
	}
}


void AiPlayer::call_action(Creature& owner, Controls key)
{
	switch (key)
	{

	case Controls::WAIT:
	{
		game.gameStatus = Game::GameStatus::NEW_TURN;
		isWaiting = true;
		break;
	}

	case Controls::TEST_COMMAND:
	{
		// Add XP for leveling up debugging (instead of spawning shopkeeper)
		if (owner.destructible)
		{
			const int DEBUG_XP_AMOUNT = 1000;
			owner.destructible->xp += DEBUG_XP_AMOUNT;
			game.message(WHITE_BLACK_PAIR, std::format("Debug: Added {} XP (Total: {})", DEBUG_XP_AMOUNT, owner.destructible->xp), true);
			
			// Use the same level up system as natural progression
			game.player->ai->levelup_update(*game.player);
		}
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
		game.message(WHITE_BLACK_PAIR, "You quit the game ! Press any key ...", true);
		break;
	}

	case Controls::ESCAPE: // if escape key is pressed bring the game menu
	{
		game.menus.push_back(std::make_unique<Menu>(false)); // false = in-game menu
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
		game.message(WHITE_BLACK_PAIR, "Which direction? (use arrow keys or numpad)", true);
		int dirKey = getch();
		Vector2D doorPos = handle_direction_input(owner, dirKey);

		if (doorPos.x != 0 || doorPos.y != 0)
		{ // Valid position
			if (game.map->is_door(doorPos))
			{
				if (game.map->open_door(doorPos))
				{
					game.message(WHITE_BLACK_PAIR, "You open the door.", true);
					game.gameStatus = Game::GameStatus::NEW_TURN;
					// FOV is recalculated inside open_door method
				}
				else
				{
					game.message(WHITE_BLACK_PAIR, "The door is already open.", true);
				}
			}
			else
			{
				game.message(WHITE_BLACK_PAIR, "There is no door there.", true);
			}
		}
		else
		{
			game.message(WHITE_BLACK_PAIR, "Invalid direction.", true);
		}
		break;
	}

	case Controls::CLOSE_DOOR:
	{
		// Prompt for direction
		game.message(WHITE_BLACK_PAIR, "Which direction? (use arrow keys or numpad)", true);
		int dirKey = getch();
		Vector2D doorPos = handle_direction_input(owner, dirKey);

		if (doorPos.x != 0 || doorPos.y != 0)
		{ // Valid position
			if (game.map->is_door(doorPos))
			{
				if (game.map->close_door(doorPos))
				{
					game.message(WHITE_BLACK_PAIR, "You close the door.", true);
					game.gameStatus = Game::GameStatus::NEW_TURN;
					// FOV is recalculated inside close_door method
				}
				else
				{
					// Try to determine why door couldn't be closed
					if (game.map->get_actor(doorPos) != nullptr)
					{
						game.message(WHITE_BLACK_PAIR, "Something is blocking the door.", true);
					}
					else
					{
						game.message(WHITE_BLACK_PAIR, "The door is already closed.", true);
					}
				}
			}
			else
			{
				game.message(WHITE_BLACK_PAIR, "There is no door there.", true);
			}
		}
		else
		{
			game.message(WHITE_BLACK_PAIR, "Invalid direction.", true);
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

	switch (dirKey)
	{
	case KEY_UP:
	case 'w': case 'W':
		delta = { -1, 0 }; // North
		break;
	case KEY_DOWN:
	case 's': case 'S':
		delta = { 1, 0 }; // South
		break;
	case KEY_LEFT:
	case 'a': case 'A':
		delta = { 0, -1 }; // West
		break;
	case KEY_RIGHT:
	case 'd': case 'D':
		delta = { 0, 1 }; // East
		break;
	case 'q': case 'Q':
		delta = { -1, -1 }; // Northwest
		break;
	case 'e': case 'E':
		delta = { -1, 1 }; // Northeast
		break;
	case 'z': case 'Z':
		delta = { 1, -1 }; // Southwest
		break;
	case 'c': case 'C':
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