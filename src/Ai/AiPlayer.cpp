// file: AiPlayer.cpp
#include <curses.h>
#include <libtcod.h>
#include <unordered_map>
#include <format>

#include "Ai.h"
#include "AiPlayer.h"
#include "AiShopkeeper.h"
#include "../Game.h"
#include "../Menu/Menu.h"
#include "../Controls/Controls.h"
#include "../Actor/Actor.h"
#include "../Actor/InventoryOperations.h"
#include "../Items/Items.h"
#include "../Items/ItemClassification.h"
#include "../ActorTypes/Gold.h"
#include "../Objects/Web.h"
#include "../Factories/ItemCreator.h"
#include "../ActorTypes/Monsters.h"
#include "../ActorTypes/Player.h"
#include "../Systems/LevelUpSystem.h"
#include "../UI/InventoryUI.h"

using namespace InventoryOperations; // For clean function calls without namespace prefix

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

void AiPlayer::update(Creature& owner, GameContext& ctx)
{
	// If stuck in a web, try to break free and skip turn if still stuck
	if (ctx.player->is_webbed())
	{
		if (!ctx.player->try_break_web(ctx))
		{
			// Still stuck, skip the player's turn
			ctx.game->gameStatus = GameStatus::NEW_TURN;
			return;
		}
	}

	const Controls key = static_cast<Controls>(ctx.input_handler->get_current_key());
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
			ctx.message_system->message(WHITE_BLACK_PAIR, "Your mind clears.", true);
		}
		else
		{
			// 50% chance of random movement
			if (ctx.dice->d2() == 1)
			{
				// Random direction
				int randomDir = ctx.dice->roll(0, 7);
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

				ctx.message_system->message(WHITE_GREEN_PAIR, "You stumble around in confusion!", true);
				ctx.game->gameStatus = GameStatus::NEW_TURN;
			}
			else
			{
				// Process normal input but show a message
				ctx.message_system->message(WHITE_GREEN_PAIR, "You struggle to control your movements...", true);

				// Check for movement keys as normal
				if (m.moves.find(key) != m.moves.end())
				{
					moveVector = m.moves.at(key);
					ctx.game->gameStatus = GameStatus::NEW_TURN;
				}
				else
				{
					// Non-movement actions proceed as normal
					Player* player = dynamic_cast<Player*>(&owner);
					if (player)
					{
						call_action(*player, key, ctx);
					}
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
			ctx.game->gameStatus = GameStatus::NEW_TURN;
		}
		else
		{
			Player* player = dynamic_cast<Player*>(&owner);
			if (player)
			{
				call_action(*player, key, ctx);
			}
		}
	}

	if (isWaiting) // when waiting tiles are triggered
	{
		isWaiting = false;
		ctx.map->tile_action(owner, ctx.map->get_tile_type(owner.position), ctx);
		look_on_floor(owner.position, ctx);
	}

	// if moving
	if (moveVector.x != 0 || moveVector.y != 0)
	{
		Vector2D targetPosition = owner.position + moveVector;
		// Only call tile_action after successful move (inside look_to_move)
		look_to_move(owner, targetPosition, ctx); // must check collisions before creature dies from attack
		look_to_attack(targetPosition, owner, ctx);
		look_on_floor(targetPosition, ctx);
		if (shouldComputeFOV)
		{
			shouldComputeFOV = false; // reset flag
			ctx.map->compute_fov(ctx);
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

void AiPlayer::pick_item(Player& player, GameContext& ctx)
{
	// Check if the inventory is already full (including equipped items)
	size_t totalItems = get_item_count(player.inventory_data);
	totalItems += player.equippedItems.size(); // Count equipped items too

	if (player.inventory_data.capacity > 0 && totalItems >= player.inventory_data.capacity)
	{
		ctx.message_system->message(WHITE_BLACK_PAIR, "Your inventory is full! You can't carry any more items.", true);
		return;
	}

	// Check if floor inventory is empty
	if (ctx.game->inventory_data.items.empty())
	{
		return; // No floor items to pick up
	}

	// Find items at player's position using range-based approach
	std::vector<std::unique_ptr<Item>*> itemsAtPosition;
	for (auto& item : ctx.game->inventory_data.items)
	{
		if (item && item->position == player.position)
		{
			itemsAtPosition.push_back(&item);
		}
	}

	if (itemsAtPosition.empty())
	{
		ctx.message_system->message(WHITE_BLACK_PAIR, "There's nothing here to pick up.", true);
		return;
	}

	// Process first item found (or implement selection for multiple items)
	auto& itemPtr = *itemsAtPosition[0];
	Item* item = itemPtr.get();

	// Handle gold directly through player's gold system
	if (item->itemClass == ItemClass::GOLD)
	{
		Gold* goldPickable = static_cast<Gold*>(item->pickable.get());
		player.adjust_gold(goldPickable->amount);
		ctx.message_system->message(YELLOW_BLACK_PAIR, "You picked up " + std::to_string(goldPickable->amount) + " gold.", true);

		// Remove gold from floor
		remove_item(ctx.game->inventory_data, *item);
		return;
	}

	// Handle regular items
	std::string itemName = item->actorData.name;
	auto result = add_item(player.inventory_data, std::move(itemPtr));

	if (result.has_value())
	{
		// Item successfully added to player inventory
		optimize_inventory_storage(ctx.game->inventory_data);
		player.sync_ranged_state(ctx);
		ctx.message_system->message(WHITE_BLACK_PAIR, "You picked up the " + itemName + ".", true);
	}
	else
	{
		// Handle inventory full error
		if (result.get_error() == InventoryError::FULL)
		{
			ctx.message_system->message(WHITE_BLACK_PAIR, "Your inventory is full!", true);
		}
		else
		{
			ctx.message_system->message(WHITE_BLACK_PAIR, "You can't pick up that item.", true);
		}
	}
}

void AiPlayer::drop_item(Player& player, GameContext& ctx)
{
	// If the inventory is empty, show a message and return
	if (player.inventory_data.items.empty())
	{
		ctx.message_system->message(WHITE_BLACK_PAIR, "Your inventory is empty!", true);
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
	if (!player.equippedItems.empty())
	{
		mvwprintw(dropWin, y++, 1, "=== EQUIPPED ===");
		
		for (const auto& equipped : player.equippedItems)
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
		
		if (!player.inventory_data.items.empty())
		{
			mvwprintw(dropWin, y++, 1, "=== INVENTORY ===");
		}
	}

	// Then show regular inventory items
	for (const auto& item : player.inventory_data.items)
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
	if (input == static_cast<int>(Controls::ESCAPE))
	{
		// Cancel dropping
		ctx.message_system->message(WHITE_BLACK_PAIR, "Drop canceled.", true);
	}
	else if (input >= 'a' && input < 'a' + static_cast<int>(player.equippedItems.size() + get_item_count(player.inventory_data)))
	{
		// Valid item selection
		int index = input - 'a';
		
		// Check if selecting equipped item
		if (index < static_cast<int>(player.equippedItems.size()))
		{
			// Dropping equipped item - unequip it first
			const auto& equipped = player.equippedItems[index];
			std::string itemName = equipped.item->actorData.name;
			
			// Unequip the item (this returns it to inventory)
			EquipmentSlot slot = equipped.slot;
			player.unequip_item(slot, ctx);
			
			// Find the item in inventory and drop it
			for (auto& item : player.inventory_data.items)
			{
				if (item && item->actorData.name == itemName)
				{
					player.drop(*item, ctx);
					break;
				}
			}
			
			ctx.message_system->message(WHITE_BLACK_PAIR, "You unequipped and dropped the " + itemName + ".", true);
		}
		else
		{
			// Dropping regular inventory item
			int inventoryIndex = index - static_cast<int>(player.equippedItems.size());

			if (inventoryIndex >= 0 && inventoryIndex < static_cast<int>(get_item_count(player.inventory_data))) {
				// Get the selected item
				Item* itemToDrop = get_item_at(player.inventory_data, inventoryIndex);
				if (itemToDrop)
				{
					// Display the item name before dropping
					std::string itemName = itemToDrop->actorData.name;

					// Drop the selected item
					player.drop(*itemToDrop, ctx);

					// Show dropped message with the item name
					ctx.message_system->message(WHITE_BLACK_PAIR, "You dropped the " + itemName + ".", true);
				}
			}
		}

		// Set game status to register the turn
		ctx.game->gameStatus = GameStatus::NEW_TURN;
	}

	// Clean up
	delwin(dropWin);

	// CRITICAL FIX: Restore the game display
	clear();
	refresh();
	ctx.game->restore_game_display();
}

bool AiPlayer::is_pickable_at_position(const Actor& actor, const Actor& owner) const
{
	return actor.position == owner.position;
}

void AiPlayer::display_inventory_items(WINDOW* inv, const Player& player) noexcept
{
	int shortcut = 'a';
	int y = 1;
	try
	{
		for (const auto& item : player.inventory_data.items)
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

void AiPlayer::display_inventory(Player& player, GameContext& ctx)
{
	InventoryUI inventoryUI;
	inventoryUI.display(player, ctx);
}

Item* AiPlayer::chose_from_inventory(Player& player, int ascii, GameContext& ctx)
{
	ctx.message_system->log("You chose from inventory");
	if (player.inventory_data.items.size() > 0)
	{
		const size_t index = ascii - 'a';
		if (index >= 0 && index < player.inventory_data.items.size())
		{
			Item* item = player.inventory_data.items.at(index).get();

			// Sync ranged state since we might use/equip/unequip an item
			player.sync_ranged_state(ctx);

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
		ctx.message_system->log("Error: choseFromInventory() called on player with no inventory.");
		exit(EXIT_FAILURE);
	}
}

void AiPlayer::look_on_floor(Vector2D target, GameContext& ctx)
{
	// look for corpses or items
	// Check if floor inventory is empty
	if (ctx.game->inventory_data.items.empty())
	{
		return;
	}

	for (const auto& i : ctx.game->inventory_data.items)
	{
		if (i)
		{
			if (i->position == target)
			{
				ctx.message_system->message(WHITE_BLACK_PAIR, "There's a " + i->actorData.name + " here\n", true);
			}
		}
	}
}

bool AiPlayer::look_to_attack(Vector2D& target, Creature& owner, GameContext& ctx)
{
	// look for living actors to attack
	for (const auto& c : ctx.game->creatures)
	{
		if (c)
		{
			if (!c->destructible->is_dead() && c->position == target)
			{
				// Check if the creature is hostile before attacking
				if (c->ai && !c->ai->is_hostile())
				{
					// Non-hostile creature - check if it's a shopkeeper for trade
					AiShopkeeper* shopkeeperAI = dynamic_cast<AiShopkeeper*>(c->ai.get());
					if (shopkeeperAI && !shopkeeperAI->tradeMenuOpen)
					{
						// Player bumped into shopkeeper - initiate trade
						ctx.message_system->log("Player bumped shopkeeper - initiating trade!");
						shopkeeperAI->trade(*c, owner, ctx);
						shopkeeperAI->tradeMenuOpen = true;
						return false; // Block movement but don't attack
					}
					else
					{
						// Other non-hostile creature - just block movement
						ctx.message_system->log("Encountered non-hostile creature: " + c->actorData.name);
						return false; // Block movement
					}
				}

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
								ctx.message_system->message(WHITE_BLACK_PAIR, "Follow-up attack: ", true);
							}
							owner.attacker->attack(owner, *c, ctx);
						}
						else
						{
							// Target died, no more attacks needed
							break;
						}
					}

					// Clean up dead creatures after combat
					ctx.game->cleanup_dead_creatures();
				}
				else
				{
				// Non-player creatures get single attack
				owner.attacker->attack(owner, *c, ctx);
				}

				// Clean up dead creatures after combat
				ctx.game->cleanup_dead_creatures();
				return false;
			}
		}
	}
	return true;
}


void AiPlayer::look_to_move(Creature& owner, const Vector2D& targetPosition, GameContext& ctx)
{
	TileType targetTileType = ctx.map->get_tile_type(targetPosition);

	if (!ctx.map->is_collision(owner, targetTileType, targetPosition, ctx))
	{
		// Check if there's a web at the target position
		bool webEffect = false;

		// Search for a web at the target position
		for (const auto& obj : *ctx.objects)
		{
			if (obj && obj->position == targetPosition &&
				obj->actorData.name == "spider web")
			{
				// Found a web, cast to proper type
				Web* web = dynamic_cast<Web*>(obj.get());
				if (web)
				{
					// Apply the web effect - returns true if player gets caught
					webEffect = web->applyEffect(owner, ctx);
					break;
				}
			}
		}

		// If the player wasn't caught in a web, proceed with movement
		if (!webEffect)
		{
			move(owner, targetPosition);
			// Call tile_action after successful move
			ctx.map->tile_action(owner, targetTileType, ctx);
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
				ctx.message_system->log("You can't swim.");
				ctx.message_system->message(WHITE_BLACK_PAIR, "You can't swim.", true);
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


void AiPlayer::call_action(Player& player, Controls key, GameContext& ctx)
{
	switch (key)
	{

	case Controls::WAIT:
	{
		ctx.game->gameStatus = GameStatus::NEW_TURN;
		isWaiting = true;
		break;
	}

	case Controls::TEST_COMMAND:
	{
		// Add XP for leveling up debugging (instead of spawning shopkeeper)
		if (player.destructible)
		{
			const int DEBUG_XP_AMOUNT = 1000;
			player.destructible->add_xp(DEBUG_XP_AMOUNT);
			ctx.message_system->message(WHITE_BLACK_PAIR, "Debug: Added " + std::to_string(DEBUG_XP_AMOUNT) + " XP (Total: " + std::to_string(player.destructible->get_xp()) + ")", true);

			// Use the same level up system as natural progression
			player.ai->levelup_update(ctx, player);
		}
		ctx.game->gameStatus = GameStatus::NEW_TURN;
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
		pick_item(player, ctx);
		ctx.game->gameStatus = GameStatus::NEW_TURN;
		break;
	}

	case Controls::DROP:
	{
		drop_item(player, ctx);
		break;
	}

	case Controls::INVENTORY:
	{
		display_inventory(player, ctx);
		break;
	}

	case Controls::QUIT:
	{
		ctx.game->run = false;
		ctx.message_system->message(WHITE_BLACK_PAIR, "You quit the game ! Press any key ...", true);
		break;
	}

	case Controls::ESCAPE: // if escape key is pressed bring the game menu
	{
		ctx.game->menus.push_back(std::make_unique<Menu>(false, ctx)); // false = in-game menu
		break;
	}

	case Controls::DESCEND:
	{
		if (ctx.game->stairs->position == player.position)
		{
			ctx.game->next_level(); // sets state to STARTUP
		}
		break;
	}

	case Controls::TARGET:
	{
		ctx.game->handle_ranged_attack();
		break;
	}

	case Controls::CHAR_SHEET:
	{
		ctx.game->display_character_sheet();
		break;
	}

	case Controls::DEBUG:
	{
		ctx.game->display_debug_messages();
		break;
	}

	case Controls::REVEAL:
	{
		ctx.map->reveal();
		break;
	}

	case Controls::REGEN:
	{
		ctx.map->regenerate(ctx);
		break;
	}

	case Controls::OPEN_DOOR:
	{
		// Prompt for direction
		ctx.message_system->message(WHITE_BLACK_PAIR, "Which direction? (use arrow keys or numpad)", true);
		int dirKey = getch();
		Vector2D doorPos = handle_direction_input(player, dirKey, ctx);

		if (doorPos.x != 0 || doorPos.y != 0)
		{ // Valid position
			if (ctx.map->is_door(doorPos))
			{
				if (ctx.map->open_door(doorPos, ctx))
				{
					ctx.message_system->message(WHITE_BLACK_PAIR, "You open the door.", true);
					ctx.game->gameStatus = GameStatus::NEW_TURN;
					// FOV is recalculated inside open_door method
				}
				else
				{
					ctx.message_system->message(WHITE_BLACK_PAIR, "The door is already open.", true);
				}
			}
			else
			{
				ctx.message_system->message(WHITE_BLACK_PAIR, "There is no door there.", true);
			}
		}
		else
		{
			ctx.message_system->message(WHITE_BLACK_PAIR, "Invalid direction.", true);
		}
		break;
	}

	case Controls::CLOSE_DOOR:
	{
		// Prompt for direction
		ctx.message_system->message(WHITE_BLACK_PAIR, "Which direction? (use arrow keys or numpad)", true);
		int dirKey = getch();
		Vector2D doorPos = handle_direction_input(player, dirKey, ctx);

		if (doorPos.x != 0 || doorPos.y != 0)
		{ // Valid position
			if (ctx.map->is_door(doorPos))
			{
				if (ctx.map->close_door(doorPos, ctx))
				{
					ctx.message_system->message(WHITE_BLACK_PAIR, "You close the door.", true);
					ctx.game->gameStatus = GameStatus::NEW_TURN;
					// FOV is recalculated inside close_door method
				}
				else
				{
					// Try to determine why door couldn't be closed
					if (ctx.map->get_actor(doorPos, ctx) != nullptr)
					{
						ctx.message_system->message(WHITE_BLACK_PAIR, "Something is blocking the door.", true);
					}
					else
					{
						ctx.message_system->message(WHITE_BLACK_PAIR, "The door is already closed.", true);
					}
				}
			}
			else
			{
				ctx.message_system->message(WHITE_BLACK_PAIR, "There is no door there.", true);
			}
		}
		else
		{
			ctx.message_system->message(WHITE_BLACK_PAIR, "Invalid direction.", true);
		}
		break;
	}

	case Controls::REST:
	{
		ctx.player->rest(ctx);
		break;
	}

	case Controls::HELP:
	{
		ctx.game->display_help();
		break;
	}

	default:
		break;
}
}

Vector2D AiPlayer::handle_direction_input(const Creature& owner, int dirKey, GameContext& ctx)
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
	if (targetPos.x < 0 || targetPos.x >= ctx.map->get_width() ||
		targetPos.y < 0 || targetPos.y >= ctx.map->get_height()) {
		return { 0, 0 }; // Out of bounds
	}

	return targetPos;
}