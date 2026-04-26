// file: PlayerController.cpp
#include <array>
#include <functional>
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
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Controls/Controls.h"
#include "../Core/GameContext.h"
#include "../Factories/ItemCreator.h"
#include "../Items/ItemClassification.h"
#include "../Map/Decoration.h"
#include "../Map/Map.h"
#include "../Menu/ContextMenu.h"
#include "../Menu/Menu.h"
#include "../Objects/Trap.h"
#include "../Persistent/Persistent.h"
#include "../Renderer/InputSystem.h"
#include "../Renderer/Renderer.h"
#include "../Systems/CreatureManager.h"
#include "../Systems/DisplayManager.h"
#include "../Systems/InputHandler.h"
#include "../Systems/LevelManager.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/SpellSystem.h"
#include "../Systems/TargetingSystem.h"
#include "../Tools/DecorEditor.h"
#include "../UI/InventoryUI.h"
#include "../Utils/Dijkstra.h"
#include "../Utils/Vector2D.h"
#include "PlayerController.h"

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Context action -- label + deferred execute, used to build right-click menus
// ---------------------------------------------------------------------------
namespace
{
struct ContextAction
{
	std::string label{};
	std::function<void(GameContext&)> execute{};
};
} // namespace

// Direction table -- static const, not a mutable global
// ---------------------------------------------------------------------------
static const std::unordered_map<Controls, Vector2D>& direction_map()
{
	static const std::unordered_map<Controls, Vector2D> moves = {
		{ Controls::UP_ARROW, DIR_N },
		{ Controls::DOWN_ARROW, DIR_S },
		{ Controls::LEFT_ARROW, DIR_W },
		{ Controls::RIGHT_ARROW, DIR_E },
		{ Controls::KP_NW, DIR_NW },
		{ Controls::KP_NE, DIR_NE },
		{ Controls::KP_SW, DIR_SW },
		{ Controls::KP_SE, DIR_SE },
	};
	return moves;
}

PlayerController::PlayerController(Player& owner)
	: playerOwner(owner)
{
}

void PlayerController::update(GameContext& ctx)
{
	// If stuck in a web, try to break free and skip turn if still stuck
	if (playerOwner.is_webbed())
	{
		if (!playerOwner.try_break_web(ctx))
		{
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
			return;
		}
	}

	if (resolve_pending_door(ctx))
	{
		return;
	}

	if (handle_mouse_path(ctx))
	{
		return;
	}

	const Controls key = static_cast<Controls>(ctx.inputHandler->get_current_key());
	Vector2D moveVector{ 0, 0 };

	// Handle confused state -- randomly move or act
	if (playerOwner.has_state(ActorState::IS_CONFUSED) && confusionTurns > 0)
	{
		confusionTurns--;

		if (confusionTurns == 0)
		{
			playerOwner.remove_state(ActorState::IS_CONFUSED);
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "Your mind clears.", true);
		}
		else
		{
			if (ctx.dice->d2() == 1)
			{
				static const std::array<Vector2D, 8> allDirections = {
					DIR_N, DIR_S, DIR_W, DIR_E, DIR_NW, DIR_NE, DIR_SW, DIR_SE
				};
				moveVector = allDirections[ctx.dice->roll(0, 7)];

				ctx.messageSystem->message(WHITE_GREEN_PAIR, "You stumble around in confusion!", true);
				ctx.gameState->set_game_status(GameStatus::NEW_TURN);
			}
			else
			{
				ctx.messageSystem->message(WHITE_GREEN_PAIR, "You struggle to control your movements...", true);

				const auto& moves = direction_map();
				if (moves.find(key) != moves.end())
				{
					moveVector = moves.at(key);
					ctx.gameState->set_game_status(GameStatus::NEW_TURN);
				}
				else
				{
					call_action(key, ctx);
				}
			}
		}
	}
	else
	{
		const auto& moves = direction_map();
		if (moves.find(key) != moves.end())
		{
			moveVector = moves.at(key);
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		}
		else
		{
			call_action(key, ctx);
		}
	}

	if (isWaiting)
	{
		isWaiting = false;
		ctx.map->tile_action(playerOwner, ctx.map->get_tile_type(playerOwner.position), ctx);
		look_on_floor(playerOwner.position, ctx);
	}

	if (moveVector.x != 0 || moveVector.y != 0)
	{
		Vector2D targetPosition = playerOwner.position + moveVector;
		look_to_move(targetPosition, ctx);
		look_to_attack(targetPosition, ctx);
		look_on_floor(targetPosition, ctx);
		flush_fov(ctx);
	}
}

void PlayerController::load(const json& /*j*/)
{
}

void PlayerController::save(json& /*j*/)
{
}

void PlayerController::move(Vector2D target)
{
	playerOwner.position = target;
}

void PlayerController::pick_item(GameContext& ctx)
{
	size_t totalItems = InventoryOperations::get_item_count(playerOwner.inventoryData);
	totalItems += playerOwner.equippedItems.size();

	if (playerOwner.inventoryData.capacity > 0 && totalItems >= playerOwner.inventoryData.capacity)
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Your inventory is full! You can't carry any more items.", true);
		return;
	}

	if (ctx.inventoryData->items.empty())
		return;

	std::vector<std::unique_ptr<Item>*> itemsAtPosition;
	for (auto& item : ctx.inventoryData->items)
	{
		if (item && item->position == playerOwner.position)
			itemsAtPosition.push_back(&item);
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
		playerOwner.adjust_gold(goldBehavior.amount);
		ctx.messageSystem->message(YELLOW_BLACK_PAIR, "You picked up " + std::to_string(goldBehavior.amount) + " gold.", true);
		InventoryOperations::remove_item(*ctx.inventoryData, *item);
		return;
	}

	std::string itemName = item->actorData.name;
	auto result = InventoryOperations::add_item(playerOwner.inventoryData, std::move(itemPtr));

	if (result.has_value())
	{
		InventoryOperations::optimize_inventory_storage(*ctx.inventoryData);
		playerOwner.sync_ranged_state(ctx);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You picked up the " + itemName + ".", true);
	}
	else
	{
		if (result.get_error() == InventoryError::FULL)
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "Your inventory is full!", true);
		else
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You can't pick up that item.", true);
	}
}

void PlayerController::drop_item(GameContext& ctx)
{
	ctx.menus->push_back(std::make_unique<InventoryUI>(playerOwner, InventoryScreen::BACKPACK, ctx));
}

bool PlayerController::is_pickable_at_position(const Actor& actor) const
{
	return actor.position == playerOwner.position;
}

void PlayerController::display_inventory(GameContext& ctx)
{
	ctx.menus->push_back(std::make_unique<InventoryUI>(playerOwner, InventoryScreen::EQUIPMENT, ctx));
}

Item* PlayerController::chose_from_inventory(int ascii, GameContext& ctx)
{
	ctx.messageSystem->log("You chose from inventory");
	if (playerOwner.inventoryData.items.size() > 0)
	{
		const size_t index = ascii - 'a';
		if (index >= 0 && index < playerOwner.inventoryData.items.size())
		{
			Item* item = playerOwner.inventoryData.items.at(index).get();
			playerOwner.sync_ranged_state(ctx);
			return item;
		}
		else
		{
			return nullptr;
		}
	}
	else
	{
		throw std::logic_error("PlayerController::chose_from_inventory -- called on player with empty inventory");
	}
}

void PlayerController::look_on_floor(Vector2D target, GameContext& ctx)
{
	if (ctx.inventoryData->items.empty())
		return;

	for (const auto& i : ctx.inventoryData->items)
	{
		if (i && i->position == target)
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "There's a " + i->actorData.name + " here", true);
	}
}

bool PlayerController::look_to_attack(Vector2D& target, GameContext& ctx)
{
	for (const auto& c : *ctx.creatures)
	{
		if (c && c->destructible)
		{
			if (!c->is_dead() && c->position == target)
			{
				if (c->ai && !c->ai->is_hostile())
				{
					if (!c->ai->is_trade_open())
					{
						ctx.messageSystem->log("Player bumped shopkeeper - initiating trade!");
						c->ai->initiate_trade(*c, playerOwner, ctx);
					}
					else
					{
						ctx.messageSystem->log("Encountered non-hostile creature: " + c->actorData.name);
					}
					return false;
				}

				playerOwner.roundCounter++;

				int attacksThisRound = 1;

				if (playerOwner.get_attacks_per_round() >= 2.0f)
				{
					attacksThisRound = 2;
				}
				else if (playerOwner.get_attacks_per_round() >= 1.5f)
				{
					attacksThisRound = (playerOwner.roundCounter % 2 == 1) ? 2 : 1;
				}

				for (int i = 0; i < attacksThisRound; i++)
				{
					if (c->destructible && !c->is_dead())
					{
						if (i > 0)
							ctx.messageSystem->message(WHITE_BLACK_PAIR, "Follow-up attack: ", true);

						playerOwner.attacker->attack(*c, ctx);
					}
					else
					{
						break;
					}
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
				if (ctx.decorEditor)
					ctx.decorEditor->erase(decor->position.x, decor->position.y);

				ctx.messageSystem->message(
					WHITE_BLACK_PAIR,
					std::format("The {} shatters!", decor->name),
					true);
				if (!decor->lootTableKey.empty())
					ctx.map->add_item(decor->position, ctx);
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

bool PlayerController::look_to_move(const Vector2D& targetPosition, GameContext& ctx)
{
	if (ctx.map->get_actor(targetPosition, ctx) != nullptr)
		return false;

	TileType targetTileType = ctx.map->get_tile_type(targetPosition);

	if (!ctx.map->is_collision(playerOwner, targetTileType, targetPosition, ctx))
	{
		bool webEffect = false;

		for (const auto& obj : *ctx.objects)
		{
			if (obj && obj->position == targetPosition)
			{
				if (obj->apply_movement_effect(playerOwner, ctx))
				{
					webEffect = true;
					break;
				}
			}
		}

		if (!webEffect)
		{
			move(targetPosition);
			ctx.map->tile_action(playerOwner, targetTileType, ctx);
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
		{
			if (!playerOwner.has_state(ActorState::CAN_SWIM))
			{
				ctx.messageSystem->log("You can't swim.");
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "You can't swim.", true);
			}
			break;
		}

		case TileType::WALL:
		{
			break;
		}

		case TileType::CLOSED_DOOR:
		{
			if (!ctx.map->is_door_locked(targetPosition))
			{
				ctx.map->open_door(targetPosition, ctx);
				ctx.gameState->set_game_status(GameStatus::NEW_TURN);
				break;
			}
			resolve_locked_door(targetPosition, ctx);
			break;
		}

		default:
		{
			break;
		}

		}
		return false;
	}
}

bool PlayerController::resolve_mouse_world_tile(GameContext& ctx, Vector2D& out_world_tile) const
{
	if (!ctx.renderer || !ctx.inputSystem)
		return false;

	int tileSize = ctx.renderer->get_tile_size();
	if (tileSize <= 0)
		return false;

	out_world_tile = ctx.inputSystem->get_mouse_world_tile(
		ctx.renderer->get_camera_x(),
		ctx.renderer->get_camera_y(),
		tileSize);

	return ctx.map->is_in_bounds(out_world_tile);
}

void PlayerController::flush_fov(GameContext& ctx)
{
	if (shouldComputeFOV)
	{
		shouldComputeFOV = false;
		ctx.map->compute_fov(ctx);
	}
}

bool PlayerController::is_mouse_pending_cancelled(GameContext& ctx) const
{
	const Controls key = static_cast<Controls>(ctx.inputHandler->get_current_key());
	const auto& moves = direction_map();
	bool isMovement = (moves.find(key) != moves.end());
	bool isAction = (key == Controls::ESCAPE || key == Controls::WAIT || key == Controls::PICK);
	return isMovement || isAction;
}

Vector2D PlayerController::find_door_approach(Vector2D doorTile, const GameContext& ctx) const
{
	if (!ctx.map)
		throw std::logic_error("PlayerController::find_door_approach -- ctx.map is null");

	const std::array<Vector2D, 4> dirs{
		Vector2D{0, -1}, Vector2D{0, 1}, Vector2D{-1, 0}, Vector2D{1, 0}
	};
	Vector2D best{ -1, -1 };
	int bestDist = INT_MAX;
	for (auto d : dirs)
	{
		Vector2D adj{ doorTile.x + d.x, doorTile.y + d.y };
		if (!ctx.map->can_walk(adj, ctx))
			continue;

		int dist = std::abs(adj.x - ctx.player->position.x)
			+ std::abs(adj.y - ctx.player->position.y);
		if (dist < bestDist)
		{
			bestDist = dist;
			best = adj;
		}
	}
	return best;
}

bool PlayerController::resolve_locked_door(Vector2D doorPos, GameContext& ctx)
{
	// Branch 1: player carries a dungeon key -- consume it, unlock, open.
	Item* keyItem = nullptr;
	for (const auto& item : playerOwner.inventoryData.items)
	{
		if (item && item->item_key == "dungeon_key")
		{
			keyItem = item.get();
			break;
		}
	}

	if (keyItem != nullptr)
	{
		InventoryOperations::remove_item(playerOwner.inventoryData, *keyItem);
		ctx.map->open_all_room_doors(doorPos, ctx);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You use the key. The lock turns.", true);
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		return true;
	}

	// Branch 2: Rogue -- Open Locks percentage roll (AD&D 2e thief skill).
	const int openLocksChance = playerOwner.get_open_locks_skill();
	if (openLocksChance > 0)
	{
		if (ctx.dice->d100() <= openLocksChance)
		{
			ctx.map->unlock_door(doorPos, ctx);
			ctx.map->open_door(doorPos, ctx);
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You pick the lock.", true);
		}
		else
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You fail to pick the lock.", true);
		}
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		return true;
	}

	// Branch 3: Fighter class or STR >= 18 -- bash the door (DC 18).
	if (playerOwner.get_creature_class() == CreatureClass::FIGHTER ||
		playerOwner.get_strength() >= 18)
	{
		constexpr int BASH_DC = 18;
		const int strRoll = ctx.dice->d20() + (playerOwner.get_strength() - 10) / 2;
		if (strRoll >= BASH_DC)
		{
			ctx.map->set_tile(doorPos, TileType::FLOOR, 1);
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You smash the door open! The crash echoes down the corridor.", true);
		}
		else
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You slam into the door but it holds.", true);
		}
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		return true;
	}

	// Branch 4: no tool available.
	ctx.messageSystem->message(WHITE_BLACK_PAIR, "The door is locked. You have no way through it.", true);
	return false;
}

void PlayerController::begin_path_walk(
	Vector2D walkDest,
	Vector2D actionTarget,
	MouseMode mode,
	PendingDoorAction doorAction,
	GameContext& ctx)
{
	if (mode == MouseMode::IDLE)
		throw std::logic_error("PlayerController::begin_path_walk -- mode must not be IDLE");

	if (!ctx.pathfinder || !ctx.map)
		return;

	auto path = ctx.pathfinder->a_star_search(
		*ctx.map, ctx.player->position, walkDest, true, ctx);

	if (path.empty())
		return;

	*ctx.mousePathOverlay = std::move(path);
	mouseMode = mode;
	mouseDoorAction = doorAction;
	mouseDoorTarget = actionTarget;
}

bool PlayerController::execute_arrival(GameContext& ctx)
{
	switch (mouseMode)
	{

	case MouseMode::WALK_TO_PICKUP:
	{
		pick_item(ctx);
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		return true;
	}

	case MouseMode::WALK_TO_DOOR:
	{
		if (mouseDoorTarget.x == -1)
			throw std::logic_error("PlayerController::execute_arrival -- WALK_TO_DOOR reached without a valid mouseDoorTarget");

		if (mouseDoorAction == PendingDoorAction::OPEN)
			ctx.map->open_door(mouseDoorTarget, ctx);
		else
			ctx.map->close_door(mouseDoorTarget, ctx);

		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		return true;
	}

	case MouseMode::WALK_TO_STAIRS:
	{
		if (ctx.stairs && ctx.stairs->position == playerOwner.position)
		{
			ctx.levelManager->advance_to_next_level(ctx);
			ctx.gameState->set_game_status(GameStatus::STARTUP);
		}
		return true;
	}

	default:
		return false;

	}
}

bool PlayerController::handle_mouse_path(GameContext& ctx)
{
	if (mouseMode == MouseMode::IDLE)
		return false;

	if (is_mouse_pending_cancelled(ctx))
	{
		ctx.mousePathOverlay->clear();
		mouseMode = MouseMode::IDLE;
		return false;
	}

	while (!ctx.mousePathOverlay->empty() && ctx.mousePathOverlay->front() == playerOwner.position)
		ctx.mousePathOverlay->erase(ctx.mousePathOverlay->begin());

	if (ctx.mousePathOverlay->empty())
	{
		bool turnConsumed = execute_arrival(ctx);
		mouseMode = MouseMode::IDLE;
		return turnConsumed;
	}

	Vector2D next = ctx.mousePathOverlay->front();
	Vector2D prevPos = playerOwner.position;
	look_to_move(next, ctx);
	look_to_attack(next, ctx);
	look_on_floor(next, ctx);
	flush_fov(ctx);

	if (playerOwner.position == next)
		ctx.mousePathOverlay->erase(ctx.mousePathOverlay->begin());

	if (playerOwner.position == prevPos)
	{
		ctx.mousePathOverlay->clear();
		mouseMode = MouseMode::IDLE;
	}

	if (ctx.mousePathOverlay->empty())
	{
		execute_arrival(ctx);
		mouseMode = MouseMode::IDLE;
	}

	if (ctx.gameState->get_game_status() != GameStatus::STARTUP)
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);

	return true;
}

void PlayerController::handle_left_click(GameContext& ctx)
{
	Vector2D world_tile;
	if (!resolve_mouse_world_tile(ctx, world_tile))
		return;

	if (world_tile == playerOwner.position)
	{
		if (ctx.stairs && ctx.stairs->position == playerOwner.position)
		{
			ctx.levelManager->advance_to_next_level(ctx);
			ctx.gameState->set_game_status(GameStatus::STARTUP);
		}
		else
		{
			pick_item(ctx);
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		}
		return;
	}

	if (ctx.map->get_actor(world_tile, ctx) != nullptr)
	{
		int dx = std::abs(playerOwner.position.x - world_tile.x);
		int dy = std::abs(playerOwner.position.y - world_tile.y);
		if (dx <= 1 && dy <= 1)
		{
			look_to_attack(world_tile, ctx);
			flush_fov(ctx);
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		}
		return;
	}

	MouseMode mode = MouseMode::WALK;
	if (ctx.stairs && ctx.stairs->position == world_tile)
	{
		mode = MouseMode::WALK_TO_STAIRS;
	}
	else
	{
		for (const auto& item : ctx.inventoryData->items)
		{
			if (item && item->position == world_tile)
			{
				mode = MouseMode::WALK_TO_PICKUP;
				break;
			}
		}
	}
	begin_path_walk(world_tile, world_tile, mode, PendingDoorAction::NONE, ctx);
}

void PlayerController::handle_right_click(GameContext& ctx)
{
	Vector2D world_tile;
	if (!resolve_mouse_world_tile(ctx, world_tile))
		return;

	int tileSize = ctx.renderer->get_tile_size();
	int anchor_col = world_tile.x - ctx.renderer->get_camera_x() / tileSize;
	int anchor_row = world_tile.y - ctx.renderer->get_camera_y() / tileSize;

	std::vector<ContextAction> actions;

	bool isPlayerTile = (world_tile == playerOwner.position);

	if (isPlayerTile)
	{
		actions.push_back({
			"Open Inventory",
			[this](GameContext& c) { display_inventory(c); }
		});
		actions.push_back({
			"Character Sheet",
			[this](GameContext& c) { c.displayManager->display_character_sheet(playerOwner, c); }
		});
		actions.push_back({
			"Rest",
			[this](GameContext& c)
			{
				playerOwner.rest(c);
			}
		});
		if (!playerOwner.memorizedSpells.empty())
		{
			actions.push_back({
				"Cast Spell",
				[this](GameContext& c) { SpellSystem::show_casting_menu(playerOwner, c); }
			});
		}
	}

	// Floor item at tile
	for (auto& item : ctx.inventoryData->items)
	{
		if (item && item->position == world_tile)
		{
			std::string itemName = item->actorData.name.substr(0, 16);
			actions.push_back({
				"Pick up " + itemName,
				[this, world_tile](GameContext& c)
				{
					begin_path_walk(
						world_tile,
						world_tile,
						MouseMode::WALK_TO_PICKUP,
						PendingDoorAction::NONE,
						c);
				}
			});
			break;
		}
	}

	// Door at tile
	bool hasDoor = ctx.map->is_door(world_tile);
	if (hasDoor)
	{
		bool doorIsOpen = ctx.map->is_open_door(world_tile);
		PendingDoorAction doorAction = doorIsOpen ? PendingDoorAction::CLOSE : PendingDoorAction::OPEN;
		std::string doorLabel = doorIsOpen ? "Close door" : "Open door";

		actions.push_back({
			doorLabel,
			[this, world_tile, doorAction](GameContext& c)
			{
				int dx = std::abs(c.player->position.x - world_tile.x);
				int dy = std::abs(c.player->position.y - world_tile.y);
				if (dx <= 1 && dy <= 1)
				{
					if (doorAction == PendingDoorAction::OPEN)
					{
						c.map->open_door(world_tile, c);
					}
					else
					{
						c.map->close_door(world_tile, c);
					}
					c.gameState->set_game_status(GameStatus::NEW_TURN);
				}
				else
				{
					Vector2D adj = find_door_approach(world_tile, c);
					if (adj.x != -1)
					{
						begin_path_walk(adj, world_tile, MouseMode::WALK_TO_DOOR, doorAction, c);
					}
				}
			}
		});
	}

	// Stairs at tile
	if (ctx.stairs && ctx.stairs->position == world_tile)
	{
		actions.push_back({
			"Descend stairs",
			[this, world_tile](GameContext& c)
			{
				begin_path_walk(
					world_tile,
					world_tile,
					MouseMode::WALK_TO_STAIRS,
					PendingDoorAction::NONE,
					c);
			}
		});
	}

	// Monster at tile (non-player)
	Creature* creature = ctx.map->get_actor(world_tile, ctx);
	if (creature && !creature->is_player())
	{
		int dx = std::abs(playerOwner.position.x - world_tile.x);
		int dy = std::abs(playerOwner.position.y - world_tile.y);
		if (dx <= 1 && dy <= 1)
		{
			std::string monName = creature->actorData.name.substr(0, 16);
			actions.push_back({
				"Attack " + monName,
				[this, world_tile](GameContext& c)
				{
					Vector2D target = world_tile;
					look_to_attack(target, c);
					flush_fov(c);
					c.gameState->set_game_status(GameStatus::NEW_TURN);
				}
			});
		}
	}

	// Walk here -- any non-player walkable tile with no creature
	if (!isPlayerTile && ctx.map->can_walk(world_tile, ctx) && creature == nullptr)
	{
		actions.push_back({
			"Walk here",
			[this, world_tile](GameContext& c)
			{
				begin_path_walk(
					world_tile,
					world_tile,
					MouseMode::WALK,
					PendingDoorAction::NONE,
					c);
			}
		});
	}

	// Nothing actionable -- skip the menu entirely
	if (actions.empty())
	{
		return;
	}

	actions.push_back({ "Cancel", [](GameContext&) {} });

	std::vector<std::string> labels;
	labels.reserve(actions.size());
	for (const auto& a : actions)
	{
		labels.push_back(a.label);
	}

	auto on_select = [actions = std::move(actions)](int sel, GameContext& c)
	{
		if (sel >= 0 && sel < static_cast<int>(actions.size()))
		{
			actions[sel].execute(c);
		}
	};

	ctx.menus->push_back(std::make_unique<ContextMenu>(
		std::move(labels), anchor_col, anchor_row, std::move(on_select), ctx));
}

void PlayerController::call_action(Controls key, GameContext& ctx)
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
		handle_left_click(ctx);
		break;
	}

	case Controls::MOUSE_RIGHT:
	{
		handle_right_click(ctx);
		break;
	}

	case Controls::PICK:
	{
		pick_item(ctx);
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		break;
	}

	case Controls::DROP:
	{
		drop_item(ctx);
		break;
	}

	case Controls::INVENTORY:
	{
		display_inventory(ctx);
		break;
	}

	case Controls::USE:
	{
		ctx.menus->push_back(std::make_unique<InventoryUI>(playerOwner, InventoryScreen::USABLES, ctx));
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
		ctx.menus->push_back(make_main_menu(false, ctx));
		break;
	}

	case Controls::DESCEND:
	{
		if (ctx.stairs->position == playerOwner.position)
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
		ctx.displayManager->display_character_sheet(playerOwner, ctx);
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
		ctx.map->spawn_all_enhanced_items_debug(playerOwner.position, ctx);
		InventoryOperations::add_item(
			playerOwner.inventoryData,
			ItemCreator::create("long_bow", playerOwner.position, *ctx.contentRegistry));
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "DEBUG: Long bow added to inventory.", true);

		playerOwner.memorizedSpells.push_back("magic_missile");
		playerOwner.memorizedSpells.push_back("magic_missile");
		playerOwner.memorizedSpells.push_back("sleep");
		playerOwner.memorizedSpells.push_back("web");
		playerOwner.memorizedSpells.push_back("teleport");
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "DEBUG: Spells added -- press Shift+C to cast.", true);
		break;
	}

	case Controls::BALANCE_VIEWER:
	{
		ctx.displayManager->display_balance_viewer(ctx);
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

	case Controls::DISARM:
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Which direction? (use arrow keys)", true);
		pendingDoorAction = PendingDoorAction::DISARM;
		break;
	}

	case Controls::REST:
	{
		playerOwner.rest(ctx);
		break;
	}

	case Controls::HIDE:
	{
		if (playerOwner.attempt_hide(ctx))
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		break;
	}

	case Controls::CAST:
	{
		SpellSystem::show_casting_menu(playerOwner, ctx);
		break;
	}

	case Controls::HELP:
	{
		ctx.displayManager->display_help(ctx);
		break;
	}

	default:
		break;

	}
}

bool PlayerController::resolve_pending_door(GameContext& ctx)
{
	if (pendingDoorAction == PendingDoorAction::NONE)
		return false;

	int dirKey = ctx.inputHandler->get_current_key();
	if (dirKey == 27)
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Cancelled.", true);
		pendingDoorAction = PendingDoorAction::NONE;
		return true;
	}
	if (dirKey == -1)
		return true;

	Vector2D doorPos = handle_direction_input(dirKey, ctx);
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
		if (!ctx.map->is_door_locked(doorPos))
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
			resolve_locked_door(doorPos, ctx);
		}
	}
	else if (pendingDoorAction == PendingDoorAction::CLOSE)
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
	else if (pendingDoorAction == PendingDoorAction::DISARM)
	{
		// Find trap at target position
		Trap* trapAtPos = nullptr;
		if (ctx.objects != nullptr)
		{
			for (auto& obj : *ctx.objects)
			{
				if (obj && obj->position == doorPos)
				{
					// Check if this is a Trap by attempting cast
					if (auto* trapPtr = dynamic_cast<Trap*>(obj.get()))
					{
						trapAtPos = trapPtr;
						break;
					}
				}
			}
		}

		if (trapAtPos == nullptr)
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "There is no trap there.", true);
		}
		else
		{
			// Attempt disarm with Dexterity check (DC from trap)
			if (trapAtPos->attempt_disarm(playerOwner, ctx))
			{
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "You successfully disarm the trap.", true);
				ctx.gameState->set_game_status(GameStatus::NEW_TURN);
			}
			else
			{
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "You fail to disarm the trap.", true);
				ctx.gameState->set_game_status(GameStatus::NEW_TURN);
			}
		}
	}

	pendingDoorAction = PendingDoorAction::NONE;
	return true;
}

Vector2D PlayerController::handle_direction_input(int dirKey, GameContext& ctx)
{
	const auto& moves = direction_map();
	const auto it = moves.find(static_cast<Controls>(dirKey));
	if (it == moves.end())
		return { 0, 0 };

	const Vector2D targetPos = playerOwner.position + it->second;
	if (!ctx.map->is_in_bounds(targetPos))
		return { 0, 0 };

	return targetPos;
}

// end of file: PlayerController.cpp
