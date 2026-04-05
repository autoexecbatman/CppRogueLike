// file: AiPlayer.cpp
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "../Actor/Actor.h"
#include "../Actor/Creature.h"
#include "../Actor/InventoryOperations.h"
#include "../Actor/Item.h"
#include "../Actor/Pickable.h"
#include "../Actor/Stairs.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Controls/Controls.h"
#include "../Core/GameContext.h"
#include "../Factories/ItemCreator.h"
#include "../Systems/ContentRegistry.h"
#include "../Items/ItemClassification.h"
#include "../Map/Decoration.h"
#include "../Map/Map.h"
#include "../Menu/ContextMenu.h"
#include "../Menu/Menu.h"
#include "../Utils/Dijkstra.h"
#include "../Objects/Web.h"
#include "../Persistent/Persistent.h"
#include "../Renderer/InputSystem.h"
#include "../Renderer/Renderer.h"
#include "../Systems/CreatureManager.h"
#include "../Systems/DisplayManager.h"
#include "../Systems/InputHandler.h"
#include "../Systems/LevelManager.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/RenderingManager.h"
#include "../Systems/SpellSystem.h"
#include "../Systems/TargetingSystem.h"
#include "../UI/InventoryUI.h"
#include "../Utils/Vector2D.h"
#include "Ai.h"
#include "AiPlayer.h"
#include "AiShopkeeper.h"

// ---------------------------------------------------------------------------
// Direction table -- static const, not a mutable global
// ---------------------------------------------------------------------------
static const std::unordered_map<Controls, Vector2D>& direction_map()
{
	static const std::unordered_map<Controls, Vector2D> moves = {
		// Arrow keys
		{ Controls::UP_ARROW, DIR_N },
		{ Controls::DOWN_ARROW, DIR_S },
		{ Controls::LEFT_ARROW, DIR_W },
		{ Controls::RIGHT_ARROW, DIR_E },

		// WASD movement
		{ Controls::W_KEY, DIR_N },
		{ Controls::S_KEY, DIR_S },
		{ Controls::A_KEY, DIR_W },
		{ Controls::D_KEY, DIR_E },

		// WASD diagonals
		{ Controls::Q_KEY, DIR_NW },
		{ Controls::E_KEY, DIR_NE },
		{ Controls::Z_KEY, DIR_SW },
		{ Controls::C_KEY, DIR_SE },
	};
	return moves;
}

void AiPlayer::update(Creature& owner, GameContext& ctx)
{
	// If stuck in a web, try to break free and skip turn if still stuck
	if (ctx.player->is_webbed())
	{
		if (!ctx.player->try_break_web(ctx))
		{
			// Still stuck, skip the player's turn
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
			return;
		}
	}

	if (resolve_pending_door(owner, ctx))
		return;

	if (handle_mouse_path(owner, ctx))
		return;

	const Controls key = static_cast<Controls>(ctx.inputHandler->get_current_key());
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
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "Your mind clears.", true);
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
				case 0:
					moveVector = { 0, -1 };
					break; // North
				case 1:
					moveVector = { 0, 1 };
					break; // South
				case 2:
					moveVector = { -1, 0 };
					break; // West
				case 3:
					moveVector = { 1, 0 };
					break; // East
				case 4:
					moveVector = { -1, -1 };
					break; // Northwest
				case 5:
					moveVector = { 1, -1 };
					break; // Northeast
				case 6:
					moveVector = { -1, 1 };
					break; // Southwest
				case 7:
					moveVector = { 1, 1 };
					break; // Southeast
				}

				ctx.messageSystem->message(WHITE_GREEN_PAIR, "You stumble around in confusion!", true);
				ctx.gameState->set_game_status(GameStatus::NEW_TURN);
			}
			else
			{
				// Process normal input but show a message
				ctx.messageSystem->message(WHITE_GREEN_PAIR, "You struggle to control your movements...", true);

				const auto& moves = direction_map();
				if (moves.find(key) != moves.end())
				{
					moveVector = moves.at(key);
					ctx.gameState->set_game_status(GameStatus::NEW_TURN);
				}
				else
				{
					Player* player = dynamic_cast<Player*>(&owner);
					if (player)
						call_action(*player, key, ctx);
				}
			}
		}
	}
	else
	{
		// Normal movement handling
		const auto& moves = direction_map();
		if (moves.find(key) != moves.end())
		{
			moveVector = moves.at(key);
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		}
		else
		{
			Player* player = dynamic_cast<Player*>(&owner);
			if (player)
				call_action(*player, key, ctx);
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
		look_to_move(owner, targetPosition, ctx);
		look_to_attack(targetPosition, owner, ctx);
		look_on_floor(targetPosition, ctx);
		flush_fov(ctx);
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
	size_t totalItems = InventoryOperations::get_item_count(player.inventoryData);
	totalItems += player.equippedItems.size();

	if (player.inventoryData.capacity > 0 && totalItems >= player.inventoryData.capacity)
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Your inventory is full! You can't carry any more items.", true);
		return;
	}

	if (ctx.inventoryData->items.empty())
	{
		return;
	}

	std::vector<std::unique_ptr<Item>*> itemsAtPosition;
	for (auto& item : ctx.inventoryData->items)
	{
		if (item && item->position == player.position)
		{
			itemsAtPosition.push_back(&item);
		}
	}

	if (itemsAtPosition.empty())
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "There's nothing here to pick up.", true);
		return;
	}

	auto& itemPtr = *itemsAtPosition[0];
	Item* item = itemPtr.get();

	if (item->itemClass == ItemClass::GOLD_COIN)
	{
		Gold& goldBehavior = std::get<Gold>(*item->behavior);
		player.adjust_gold(goldBehavior.amount);
		ctx.messageSystem->message(YELLOW_BLACK_PAIR, "You picked up " + std::to_string(goldBehavior.amount) + " gold.", true);
		InventoryOperations::remove_item(*ctx.inventoryData, *item);
		return;
	}

	std::string itemName = item->actorData.name;
	auto result = InventoryOperations::add_item(player.inventoryData, std::move(itemPtr));

	if (result.has_value())
	{
		InventoryOperations::optimize_inventory_storage(*ctx.inventoryData);
		player.sync_ranged_state(ctx);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You picked up the " + itemName + ".", true);
	}
	else
	{
		if (result.get_error() == InventoryError::FULL)
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "Your inventory is full!", true);
		}
		else
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You can't pick up that item.", true);
		}
	}
}

void AiPlayer::drop_item(Player& player, GameContext& ctx)
{
	if (player.inventoryData.items.empty())
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Your inventory is empty!", true);
		return;
	}

	// TODO: Reimplement drop_item UI without curses (was a curses WINDOW-based menu)
	if (!player.inventoryData.items.empty())
	{
		Item* itemToDrop = player.inventoryData.items.front().get();
		if (itemToDrop)
		{
			std::string itemName = itemToDrop->actorData.name;
			player.drop(*itemToDrop, ctx);
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You dropped the " + itemName + ".", true);
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		}
	}

	ctx.renderingManager->restore_game_display();
}

bool AiPlayer::is_pickable_at_position(const Actor& actor, const Actor& owner) const
{
	return actor.position == owner.position;
}

void AiPlayer::display_inventory_items(void* /*inv*/, const Player& /*player*/) noexcept
{
	// TODO: Reimplement display_inventory_items without curses
}

void AiPlayer::display_inventory(Player& player, GameContext& ctx)
{
	ctx.menus->push_back(std::make_unique<InventoryUI>(player, InventoryScreen::EQUIPMENT, ctx));
}

Item* AiPlayer::chose_from_inventory(Player& player, int ascii, GameContext& ctx)
{
	ctx.messageSystem->log("You chose from inventory");
	if (player.inventoryData.items.size() > 0)
	{
		const size_t index = ascii - 'a';
		if (index >= 0 && index < player.inventoryData.items.size())
		{
			Item* item = player.inventoryData.items.at(index).get();
			player.sync_ranged_state(ctx);
			return item;
		}
		else
		{
			return nullptr;
		}
	}
	else
	{
		throw std::logic_error("AiPlayer::chose_from_inventory -- called on player with empty inventory");
	}
}

void AiPlayer::look_on_floor(Vector2D target, GameContext& ctx)
{
	if (ctx.inventoryData->items.empty())
	{
		return;
	}

	for (const auto& i : ctx.inventoryData->items)
	{
		if (i && i->position == target)
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "There's a " + i->actorData.name + " here", true);
		}
	}
}

bool AiPlayer::look_to_attack(Vector2D& target, Creature& owner, GameContext& ctx)
{
	for (const auto& c : *ctx.creatures)
	{
		if (c && c->destructible)
		{
			if (!c->destructible->is_dead() && c->position == target)
			{
				if (c->ai && !c->ai->is_hostile())
				{
					AiShopkeeper* shopkeeperAI = dynamic_cast<AiShopkeeper*>(c->ai.get());
					if (shopkeeperAI && !shopkeeperAI->tradeMenuOpen)
					{
						ctx.messageSystem->log("Player bumped shopkeeper - initiating trade!");
						shopkeeperAI->trade(*c, owner, ctx);
						shopkeeperAI->tradeMenuOpen = true;
						return false;
					}
					else
					{
						ctx.messageSystem->log("Encountered non-hostile creature: " + c->actorData.name);
						return false;
					}
				}

				auto playerPtr = dynamic_cast<Player*>(&owner);
				if (playerPtr)
				{
					playerPtr->roundCounter++;

					int attacksThisRound = 1;

					if (playerPtr->get_attacks_per_round() >= 2.0f)
					{
						attacksThisRound = 2;
					}
					else if (playerPtr->get_attacks_per_round() >= 1.5f)
					{
						attacksThisRound = (playerPtr->roundCounter % 2 == 1) ? 2 : 1;
					}

					for (int i = 0; i < attacksThisRound; i++)
					{
						if (c->destructible && !c->destructible->is_dead())
						{
							if (i > 0)
							{
								ctx.messageSystem->message(WHITE_BLACK_PAIR, "Follow-up attack: ", true);
							}
							owner.attacker->attack(owner, *c, ctx);
						}
						else
						{
							break;
						}
					}

					ctx.creatureManager->cleanup_dead_creatures(*ctx.creatures);
				}
				else
				{
					owner.attacker->attack(owner, *c, ctx);
				}

				ctx.creatureManager->cleanup_dead_creatures(*ctx.creatures);
				return false;
			}
		}
	}

	if (ctx.map && ctx.decorations)
	{
		auto* decor = ctx.map->find_decoration_at(target, ctx);
		if (decor && !decor->isBroken)
		{
			--decor->hp;
			if (decor->hp <= 0)
			{
				decor->isBroken = true;
				ctx.messageSystem->message(
					WHITE_BLACK_PAIR,
					std::format("The {} shatters!", decor->name),
					true);
				if (!decor->lootTableKey.empty())
				{
					ctx.map->add_item(decor->position, ctx);
				}
			}
			else
			{
				ctx.messageSystem->message(
					WHITE_BLACK_PAIR,
					std::format("You hit the {}.", decor->name),
					true);
			}
			return false;
		}
	}

	return true;
}

bool AiPlayer::look_to_move(Creature& owner, const Vector2D& targetPosition, GameContext& ctx)
{
	TileType targetTileType = ctx.map->get_tile_type(targetPosition);

	if (!ctx.map->is_collision(owner, targetTileType, targetPosition, ctx))
	{
		bool webEffect = false;

		for (const auto& obj : *ctx.objects)
		{
			if (obj && obj->position == targetPosition &&
				obj->actorData.name == "spider web")
			{
				Web* web = dynamic_cast<Web*>(obj.get());
				if (web)
				{
					webEffect = web->apply_effect(owner, ctx);
					break;
				}
			}
		}

		if (!webEffect)
		{
			move(owner, targetPosition);
			ctx.map->tile_action(owner, targetTileType, ctx);
			shouldComputeFOV = true;
			return true;
		}
		return false;
	}
	else
	{
		switch (targetTileType)
		{
		case TileType::WATER:
			if (!owner.has_state(ActorState::CAN_SWIM))
			{
				ctx.messageSystem->log("You can't swim.");
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "You can't swim.", true);
			}
			break;
		case TileType::WALL:
			break;
		case TileType::CLOSED_DOOR:
			break;
		default:
			break;
		}
		return false;
	}
}

bool AiPlayer::resolve_mouse_world_tile(GameContext& ctx, Vector2D& out_world_tile) const
{
	if (!ctx.renderer || !ctx.inputSystem)
	{
		return false;
	}
	int tileSize = ctx.renderer->get_tile_size();
	if (tileSize <= 0)
	{
		return false;
	}
	out_world_tile = ctx.inputSystem->get_mouse_world_tile(
		ctx.renderer->get_camera_x(),
		ctx.renderer->get_camera_y(),
		tileSize);
	return ctx.map->is_in_bounds(out_world_tile);
}

void AiPlayer::flush_fov(GameContext& ctx)
{
	if (shouldComputeFOV)
	{
		shouldComputeFOV = false;
		ctx.map->compute_fov(ctx);
	}
}

bool AiPlayer::is_mouse_pending_cancelled(GameContext& ctx) const
{
	const Controls key = static_cast<Controls>(ctx.inputHandler->get_current_key());
	const auto& moves = direction_map();
	bool isMovement = (moves.find(key) != moves.end());
	bool isAction = (key == Controls::ESCAPE || key == Controls::WAIT || key == Controls::PICK);
	return isMovement || isAction;
}

// ---------------------------------------------------------------------------
// begin_path_walk -- compute A* path and enter a mouse navigation mode.
// Uses includeGoalIfBlocked=true so doors (impassable) appear as reachable
// final nodes; handle_mouse_path detects the block and executes door action.
// ---------------------------------------------------------------------------
void AiPlayer::begin_path_walk(
	Vector2D destination,
	MouseMode mode,
	PendingDoorAction doorAction,
	GameContext& ctx)
{
	if (!ctx.pathfinder || !ctx.map)
	{
		return;
	}
	auto path = ctx.pathfinder->a_star_search(
		*ctx.map, ctx.player->position, destination, true, ctx);
	if (path.empty())
	{
		return;
	}
	*ctx.mousePathOverlay = std::move(path);
	mouseMode = mode;
	mouseDoorAction = doorAction;
	ctx.gameState->set_game_status(GameStatus::NEW_TURN);
}

// ---------------------------------------------------------------------------
// execute_arrival -- called when the player reaches the path destination.
// Returns true if a game turn was consumed.
// ---------------------------------------------------------------------------
bool AiPlayer::execute_arrival(Creature& owner, GameContext& ctx)
{
	switch (mouseMode)
	{
	case MouseMode::WALK_TO_PICKUP:
	{
		Player* player = dynamic_cast<Player*>(&owner);
		if (!player)
		{
			throw std::logic_error("AiPlayer::execute_arrival -- owner is not a Player");
		}
		pick_item(*player, ctx);
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		return true;
	}
	case MouseMode::WALK_TO_DOOR:
	{
		// The remaining path front is the door tile
		if (!ctx.mousePathOverlay->empty())
		{
			Vector2D doorPos = ctx.mousePathOverlay->front();
			if (mouseDoorAction == PendingDoorAction::OPEN)
			{
				ctx.map->open_door(doorPos, ctx);
			}
			else
			{
				ctx.map->close_door(doorPos, ctx);
			}
		}
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		return true;
	}
	default:
		return false;
	}
}

// ---------------------------------------------------------------------------
// handle_mouse_path -- unified walk handler for all mouse navigation modes.
// Rate-limiting (pacing between steps) lives in GameLoopCoordinator;
// this function executes exactly one step per call.
// ---------------------------------------------------------------------------
bool AiPlayer::handle_mouse_path(Creature& owner, GameContext& ctx)
{
	if (mouseMode == MouseMode::IDLE)
	{
		return false;
	}

	if (is_mouse_pending_cancelled(ctx))
	{
		ctx.mousePathOverlay->clear();
		mouseMode = MouseMode::IDLE;
		return false;
	}

	// Drain nodes where the owner is already standing
	while (!ctx.mousePathOverlay->empty() && ctx.mousePathOverlay->front() == owner.position)
	{
		ctx.mousePathOverlay->erase(ctx.mousePathOverlay->begin());
	}

	if (ctx.mousePathOverlay->empty())
	{
		bool turnConsumed = execute_arrival(owner, ctx);
		mouseMode = MouseMode::IDLE;
		return turnConsumed;
	}

	Vector2D next = ctx.mousePathOverlay->front();
	Vector2D prevPos = owner.position;
	look_to_move(owner, next, ctx);
	look_to_attack(next, owner, ctx);
	look_on_floor(next, ctx);
	flush_fov(ctx);

	if (owner.position == next)
	{
		ctx.mousePathOverlay->erase(ctx.mousePathOverlay->begin());
	}

	if (owner.position == prevPos)
	{
		// Blocked: if the one remaining node is a door, execute door arrival
		if (mouseMode == MouseMode::WALK_TO_DOOR && ctx.mousePathOverlay->size() == 1)
		{
			execute_arrival(owner, ctx);
		}
		ctx.mousePathOverlay->clear();
		mouseMode = MouseMode::IDLE;
	}

	if (ctx.mousePathOverlay->empty())
	{
		mouseMode = MouseMode::IDLE;
	}

	ctx.gameState->set_game_status(GameStatus::NEW_TURN);
	return true;
}

void AiPlayer::call_action(Player& player, Controls key, GameContext& ctx)
{
	switch (key)
	{

	case Controls::WAIT:
	{
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		isWaiting = true;
		break;
	}

	case Controls::MOUSE:
	{
		Vector2D world_tile;
		if (!resolve_mouse_world_tile(ctx, world_tile))
		{
			break;
		}

		if (world_tile == player.position)
		{
			pick_item(player, ctx);
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
			break;
		}

		bool hasDoor = ctx.map->is_door(world_tile);
		if (hasDoor)
		{
			int dx = std::abs(player.position.x - world_tile.x);
			int dy = std::abs(player.position.y - world_tile.y);
			if (dx <= 1 && dy <= 1)
			{
				// Already adjacent -- act immediately
				if (ctx.map->is_open_door(world_tile))
				{
					ctx.map->close_door(world_tile, ctx);
				}
				else
				{
					ctx.map->open_door(world_tile, ctx);
				}
				ctx.gameState->set_game_status(GameStatus::NEW_TURN);
				break;
			}
			PendingDoorAction action = ctx.map->is_open_door(world_tile)
				? PendingDoorAction::CLOSE
				: PendingDoorAction::OPEN;
			begin_path_walk(world_tile, MouseMode::WALK_TO_DOOR, action, ctx);
			break;
		}

		// If a creature occupies the tile, attack if adjacent -- never auto-walk to a moving target
		if (ctx.map->get_actor(world_tile, ctx) != nullptr)
		{
			int dx = std::abs(player.position.x - world_tile.x);
			int dy = std::abs(player.position.y - world_tile.y);
			if (dx <= 1 && dy <= 1)
			{
				look_to_attack(world_tile, player, ctx);
				flush_fov(ctx);
				ctx.gameState->set_game_status(GameStatus::NEW_TURN);
			}
			break;
		}

		begin_path_walk(world_tile, MouseMode::WALK, PendingDoorAction::NONE, ctx);
		break;
	}

	case Controls::MOUSE_RIGHT:
	{
		Vector2D world_tile;
		if (!resolve_mouse_world_tile(ctx, world_tile))
		{
			break;
		}

		int tileSize = ctx.renderer->get_tile_size();

		Item* foundItem = nullptr;
		for (auto& item : ctx.inventoryData->items)
		{
			if (item && item->position == world_tile)
			{
				foundItem = item.get();
				break;
			}
		}

		bool hasDoor = ctx.map->is_door(world_tile);
		bool doorIsOpen = hasDoor && ctx.map->is_open_door(world_tile);

		if (foundItem || hasDoor)
		{
			std::vector<std::string> options;
			if (foundItem)
			{
				options.push_back("Pick up " + foundItem->actorData.name.substr(0, 14));
			}
			if (hasDoor)
			{
				options.push_back(doorIsOpen ? "Close door" : "Open door");
			}
			options.push_back("Cancel");

			int anchor_col = world_tile.x - ctx.renderer->get_camera_x() / tileSize;
			int anchor_row = world_tile.y - ctx.renderer->get_camera_y() / tileSize;
			ContextMenu contextMenu{ std::move(options), anchor_col, anchor_row, ctx };
			contextMenu.menu(ctx);
			int sel = contextMenu.get_selected();

			int idx = 0;
			if (foundItem)
			{
				if (sel == idx)
				{
					begin_path_walk(world_tile, MouseMode::WALK_TO_PICKUP, PendingDoorAction::NONE, ctx);
				}
				idx++;
			}
			if (hasDoor)
			{
				if (sel == idx)
				{
					PendingDoorAction action = doorIsOpen ? PendingDoorAction::CLOSE : PendingDoorAction::OPEN;
					begin_path_walk(world_tile, MouseMode::WALK_TO_DOOR, action, ctx);
				}
				// last index = Cancel -- do nothing
			}
		}
		break;
	}

	case Controls::PICK:
	{
		pick_item(player, ctx);
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
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

	case Controls::USE:
	{
		ctx.menus->push_back(std::make_unique<InventoryUI>(player, InventoryScreen::USABLES, ctx));
		break;
	}

	case Controls::QUIT:
	{
		ctx.gameState->set_run(false);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You quit the game ! Press any key ...", true);
		break;
	}

	case Controls::ESCAPE:
	{
		ctx.menus->push_back(std::make_unique<Menu>(false, ctx));
		break;
	}

	case Controls::DESCEND:
	{
		if (ctx.stairs->position == player.position)
		{
			ctx.levelManager->advance_to_next_level(ctx);
			ctx.gameState->set_game_status(GameStatus::STARTUP);
		}
		break;
	}

	case Controls::TARGET:
	{
		ctx.targeting->handle_ranged_attack(ctx);
		break;
	}

	case Controls::CHAR_SHEET:
	{
		ctx.displayManager->display_character_sheet(*ctx.player, ctx);
		break;
	}

#ifndef NDEBUG
	case Controls::DEBUG:
	{
		ctx.messageSystem->display_debug_messages();
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

	case Controls::TEST_COMMAND:
	{
		ctx.map->spawn_all_enhanced_items_debug(player.position, ctx);
		InventoryOperations::add_item(
			player.inventoryData,
			ItemCreator::create("long_bow", player.position, *ctx.contentRegistry));
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "DEBUG: Long bow added to inventory.", true);

		player.memorizedSpells.push_back("magic_missile");
		player.memorizedSpells.push_back("magic_missile");
		player.memorizedSpells.push_back("sleep");
		player.memorizedSpells.push_back("web");
		player.memorizedSpells.push_back("teleport");
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "DEBUG: Spells added -- press Shift+C to cast.", true);
		break;
	}

	case Controls::ITEM_DISTRIBUTION:
	{
		ctx.map->display_item_distribution(ctx);
		break;
	}
#endif

	case Controls::OPEN_DOOR:
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Which direction? (use arrow keys)", true);
		pendingDoorAction = PendingDoorAction::OPEN;
		break;
	}

	case Controls::CLOSE_DOOR:
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Which direction? (use arrow keys)", true);
		pendingDoorAction = PendingDoorAction::CLOSE;
		break;
	}

	case Controls::REST:
	{
		ctx.player->rest(ctx);
		break;
	}

	case Controls::HIDE:
	{
		if (player.attempt_hide(ctx))
		{
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		}
		break;
	}

	case Controls::CAST:
	{
		SpellSystem::show_casting_menu(player, ctx);
		break;
	}

	case Controls::HELP:
	{
		ctx.displayManager->display_help();
		break;
	}

	default:
		break;
	}
}

bool AiPlayer::resolve_pending_door(Creature& owner, GameContext& ctx)
{
	if (pendingDoorAction == PendingDoorAction::NONE)
	{
		return false;
	}

	int dirKey = ctx.inputHandler->get_current_key();
	if (dirKey == 27)
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Cancelled.", true);
		pendingDoorAction = PendingDoorAction::NONE;
		return true;
	}
	if (dirKey == -1)
	{
		return true;
	}

	Vector2D doorPos = handle_direction_input(owner, dirKey, ctx);
	if (doorPos.x == 0 && doorPos.y == 0)
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Invalid direction.", true);
		pendingDoorAction = PendingDoorAction::NONE;
		return true;
	}

	if (!ctx.map->is_door(doorPos))
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "There is no door there.", true);
		pendingDoorAction = PendingDoorAction::NONE;
		return true;
	}

	if (pendingDoorAction == PendingDoorAction::OPEN)
	{
		if (ctx.map->open_door(doorPos, ctx))
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You open the door.", true);
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		}
		else
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "The door is already open.", true);
		}
	}
	else
	{
		if (ctx.map->close_door(doorPos, ctx))
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You close the door.", true);
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		}
		else if (ctx.map->get_actor(doorPos, ctx) != nullptr)
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "Something is blocking the door.", true);
		}
		else
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "The door is already closed.", true);
		}
	}

	pendingDoorAction = PendingDoorAction::NONE;
	return true;
}

Vector2D AiPlayer::handle_direction_input(const Creature& owner, int dirKey, GameContext& ctx)
{
	const auto& moves = direction_map();
	const auto it = moves.find(static_cast<Controls>(dirKey));
	if (it == moves.end())
	{
		return { 0, 0 };
	}

	const Vector2D targetPos = owner.position + it->second;
	if (!ctx.map->is_in_bounds(targetPos))
	{
		return { 0, 0 };
	}

	return targetPos;
}
