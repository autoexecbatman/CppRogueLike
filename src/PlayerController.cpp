// file: PlayerController.cpp
#include <array>
#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Actor.h"
#include "AttackKind.h"
#include "AttackRate.h"
#include "Colors.h"
#include "ContextMenu.h"
#include "Controls.h"
#include "CostOverlay.h"
#include "Creature.h"
#include "CreatureManager.h"
#include "DataManager.h"
#include "DecorEditor.h"
#include "Decoration.h"
#include "DisplayManager.h"
#include "GameContext.h"
#include "InputHandler.h"
#include "InputSystem.h"
#include "InventoryOperations.h"
#include "InventoryUI.h"
#include "Item.h"
#include "ItemCreator.h"
#include "ItemFactory.h"
#include "LevelManager.h"
#include "ListMenu.h"
#include "Map.h"
#include "Menu.h"
#include "MenuEntry.h"
#include "MenuTrade.h"
#include "MessageSystem.h"
#include "MonsterCreator.h"
#include "MonsterRegistry.h"
#include "Pathfinding.h"
#include "Persistent.h"
#include "Pickup.h"
#include "Player.h"
#include "PlayerController.h"
#include "PlayerTurn.h"
#include "Renderer.h"
#include "SpellSystem.h"
#include "SpellTile.h"
#include "TargetingSystem.h"
#include "ThiefSkills.h"
#include "TileConfig.h"
#include "Trap.h"
#include "TurnUndead.h"
#include "Vector2D.h"

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
	assert(ctx.inputHandler && "PlayerController::update: called without an inputHandler");
	assert(ctx.map && "PlayerController::update: called without a map");
	assert(ctx.dice && "PlayerController::update: called without dice");

	// If stuck in a web, try to break free and skip turn if still stuck
	if (playerOwner.is_webbed())
	{
		const WebEscape escape = playerOwner.try_break_web(ctx);

		switch (escape)
		{
		case WebEscape::BROKE_FREE:
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You break free from the web!", MessageCompletion::FINISHED);
			break;
		}
		case WebEscape::STRUGGLED_FREE:
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You finally break free from the web!", MessageCompletion::FINISHED);
			break;
		}
		case WebEscape::STILL_STUCK:
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You're still stuck in the web.", MessageCompletion::FINISHED);
			spend_player_action(ctx, TIME_UNITS_PER_ROUND);
			return;
		}
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

	// A pending confusion is either still in force or has ended since the player last acted.
	if (confusionEndTime.has_value())
	{
		if (!is_confused(ctx.gameState->get_time()))
		{
			confusionEndTime.reset();
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Your mind clears.", MessageCompletion::FINISHED);
		}
		else
		{
			if (ctx.dice->d2() == 1)
			{
				static const std::array<Vector2D, 8> allDirections = {
					DIR_N,
					DIR_S,
					DIR_W,
					DIR_E,
					DIR_NW,
					DIR_NE,
					DIR_SW,
					DIR_SE,
				};
				moveVector = allDirections[ctx.dice->roll(0, 7)];

				ctx.messageSystem->message(ColorPairId::WHITE_GREEN, "You stumble around in confusion!", MessageCompletion::FINISHED);
				spend_player_action(ctx, TIME_UNITS_PER_ROUND);
			}
			else
			{
				ctx.messageSystem->message(ColorPairId::WHITE_GREEN, "You struggle to control your movements...", MessageCompletion::FINISHED);

				const auto& moves = direction_map();
				if (moves.contains(key))
				{
					moveVector = moves.at(key);
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
		if (moves.contains(key))
		{
			moveVector = moves.at(key);
		}
		else
		{
			call_action(key, ctx);
		}
	}

	if (isWaiting)
	{
		isWaiting = false;
		ctx.map->describe_tile(ctx.map->get_tile_type(playerOwner.position), ctx);
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

// Almost all of the controller is frame state - the field-of-view flag, the waiting
// flag, the pending door action and the whole mouse path - and is either rebuilt each
// frame or is mid-interaction state that cannot exist at the moment a game is saved.
// The confusion is the exception: it is a reading on the shared clock, and a spell
// still running when the player saves is still running when they come back.
void PlayerController::load(const json& savedState)
{
	const json& endTime = savedState.at("confusionEndTime");
	confusionEndTime = endTime.is_null() ? std::nullopt : std::optional<int>{ endTime.get<int>() };
}

void PlayerController::save(json& savedState)
{
	savedState["confusionEndTime"] = confusionEndTime.has_value() ? json(*confusionEndTime) : json(nullptr);
}

void PlayerController::move(Vector2D target)
{
	playerOwner.position = target;
}

// The player asks for whatever is underfoot. Pickup owns what that means, including
// the menu a heap needs, so the controller only names the actor doing it.
void PlayerController::pick_item(GameContext& ctx)
{
	Pickup::from_floor(playerOwner, ctx);
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
	assert(ctx.floorInventory && "PlayerController::look_on_floor: called without a floorInventory");

	if (ctx.floorInventory->items.empty())
	{
		return;
	}

	for (const auto& floorItem : ctx.floorInventory->items)
	{
		assert(floorItem && "look_on_floor: the floor holds a null where an item should be");

		if (floorItem->position == target)
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "There's a " + floorItem->get_name() + " here", MessageCompletion::FINISHED);
		}
	}
}

bool PlayerController::look_to_attack(Vector2D& target, GameContext& ctx)
{
	assert(ctx.creatures && "PlayerController::look_to_attack: called without creatures");

	for (const auto& creature : *ctx.creatures)
	{
		assert(creature != nullptr && "null creature in creatures list");
		if (!creature->is_dead() && creature->position == target)
		{
			assert(creature->ai != nullptr && "living creature at target has no AI");

			// Bumping anything the player would not attack outright resolves as
			// an interaction rather than a swing.
			if (needs_attack_confirmation(creature->get_attitude()))
			{
				resolve_peaceful_bump(*creature, ctx);
				spend_player_action(ctx, TIME_UNITS_PER_ROUND);
				return false;
			}

			strike(*creature, ctx);
			return false;
		}
	}

	assert(ctx.map && "PlayerController::look_to_attack: called without a map");
	assert(ctx.decorations && "PlayerController::look_to_attack: called without decorations");
	auto* decor = ctx.map->find_decoration_at(target, ctx);
	if (decor && !decor->isBroken)
	{
		--decor->hp;
		if (decor->hp <= 0)
		{
			decor->isBroken = true;
			assert(ctx.decorEditor && "PlayerController::look_to_attack: called without a decorEditor");
			ctx.decorEditor->erase(decor->position);

			ctx.messageSystem->message(
				ColorPairId::WHITE_BLACK,
				std::format("The {} shatters!", decor->name),
				MessageCompletion::FINISHED);
			if (!decor->lootTableKey.empty())
			{
				ctx.map->add_item(decor->position, ctx);
			}
		}
		else
		{
			ctx.messageSystem->message(
				ColorPairId::WHITE_BLACK,
				std::format("You hit the {}.", decor->name),
				MessageCompletion::FINISHED);
		}
		return false;
	}

	return true;
}

// Handles bumping a creature the player would not attack outright.
//
// Shopkeepers trade. Anything else displaceable swaps places, which is how a
// peaceful creature is passed rather than fought. A creature that is neither
// simply blocks; attacking it is a separate deliberate command.
//
// Example:
//   resolve_peaceful_bump(shopkeeper, ctx); // opens the trade menu
//   resolve_peaceful_bump(villager, ctx);   // player and villager swap tiles
void PlayerController::resolve_peaceful_bump(Creature& target, GameContext& ctx)
{
	// A shopkeeper's answer to being bumped is its shop.
	if (target.shop)
	{
		ctx.messageSystem->log("Player bumped shopkeeper - initiating trade!");
		open_trade(target, playerOwner, ctx);
		return;
	}

	// A creature that will not step aside can only be passed by force, so the
	// bump becomes the attack prompt.
	if (!target.is_displaceable())
	{
		confirm_attack_on_peaceful(target, ctx);
		return;
	}

	swap_places_with(target, ctx);
}

// Player and target trade tiles, so a peaceful creature is passed rather than
// fought. Only reached for a creature that consents to being displaced.
//
// Example:
//   swap_places_with(villager, ctx); // player stands where the villager did
void PlayerController::swap_places_with(Creature& target, GameContext& ctx)
{
	const Vector2D targetPosition = target.position;

	target.position = playerOwner.position;
	move(targetPosition);

	spend_player_action(ctx, TIME_UNITS_PER_ROUND);
}

// Channels the player's deity against nearby undead and narrates the result.
//
// The system decides what happened; this decides what the player is told, and
// spends the turn only when an attempt was actually made.
void PlayerController::attempt_turn_undead(GameContext& ctx)
{
	assert(ctx.creatureManager && "PlayerController::attempt_turn_undead: called without a creatureManager");
	assert(ctx.creatures && "PlayerController::attempt_turn_undead: called without creatures");

	const Turning::TurnUndeadReport report = Turning::turn_undead(playerOwner, ctx);

	if (!report.attempted)
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You have no deity to channel.", MessageCompletion::FINISHED);
		return;
	}

	if (report.turned.empty() && report.destroyed.empty() && report.resisted.empty())
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You hold up your holy symbol, but nothing stirs.", MessageCompletion::FINISHED);
		return;
	}

	ctx.messageSystem->message(ColorPairId::YELLOW_BLACK, "You raise your holy symbol!", MessageCompletion::FINISHED);

	for (const Creature* destroyed : report.destroyed)
	{
		ctx.messageSystem->message(
			ColorPairId::GREEN_BLACK,
			std::format("The {} crumbles to dust!", destroyed->actorData.name),
			MessageCompletion::FINISHED);
	}

	for (const Creature* turned : report.turned)
	{
		ctx.messageSystem->message(
			ColorPairId::WHITE_BLACK,
			std::format("The {} recoils and flees!", turned->actorData.name),
			MessageCompletion::FINISHED);
	}

	for (const Creature* resisted : report.resisted)
	{
		ctx.messageSystem->message(
			ColorPairId::RED_BLACK,
			std::format("The {} is unmoved.", resisted->actorData.name),
			MessageCompletion::FINISHED);
	}

	ctx.creatureManager->cleanup_dead_creatures(*ctx.creatures);
	spend_player_action(ctx, TIME_UNITS_PER_ROUND);
}

// Asks before striking a creature that has not threatened the player, and
// applies the consequence if the answer is yes.
//
// Turning on the peaceable is the player's choice, so it is never silent: the
// prompt is the guard, and the alignment shift is the cost.
void PlayerController::confirm_attack_on_peaceful(Creature& target, GameContext& ctx)
{
	std::vector<MenuEntry> entries{};

	// Init-capture: the menu outlives this call, so the target is captured by
	// pointer rather than through a reference that would dangle on return.
	auto attackCommand = [this, victim = &target](GameContext& menuCtx)
	{
		Player& player = menuCtx.player_concrete();

		// House rule: betraying the peaceable costs one step toward chaotic.
		player.set_ethics(shift_toward_chaotic(player.get_ethics()));

		// The victim stops being peaceable the moment it is struck.
		victim->set_attitude(Attitude::HOSTILE);

		menuCtx.messageSystem->message(
			ColorPairId::RED_BLACK,
			std::format("You attack the {}!", victim->actorData.name),
			MessageCompletion::FINISHED);

		strike(*victim, menuCtx);
		menuCtx.menus->back()->back = true;
	};

	auto cancelCommand = [](GameContext& menuCtx)
	{
		menuCtx.menus->back()->back = true;
	};

	entries.push_back({ "Yes, attack", 'y', attackCommand });
	entries.push_back({ "No", 'n', cancelCommand });

	ctx.menus->push_back(std::make_unique<ListMenu>(
		std::format("Really attack the {}?", target.actorData.name),
		std::move(entries),
		std::function<void(GameContext&)>{},
		std::function<void(GameContext&)>{},
		ctx));
}

// Resolves one round of the player's melee against a target, including the
// follow-up attacks a high attacks-per-round buys.
//
// Example:
//   strike(goblin, ctx); // one swing, or two at 2.0 attacks per round
void PlayerController::strike(Creature& target, GameContext& ctx)
{
	assert(ctx.creatureManager && "PlayerController::strike: called without a creatureManager");
	assert(ctx.creatures && "PlayerController::strike: called without creatures");
	assert(playerOwner.attacker && "strike: the player swung with no attacker");

	// One swing, priced by the rate, so a faster attacker comes round sooner (Table 58).
	playerOwner.attacker->attack(target, AttackKind::MELEE, ctx);
	spend_player_action(ctx, attack_cost(playerOwner.get_attacks_per_round()));
	ctx.creatureManager->cleanup_dead_creatures(*ctx.creatures);
}

bool PlayerController::look_to_move(const Vector2D& targetPosition, GameContext& ctx)
{
	assert(ctx.tileConfig && "PlayerController::look_to_move: called without a tileConfig");
	assert(ctx.map && "PlayerController::look_to_move: called without a map");

	if (ctx.map->get_actor(targetPosition, ctx) != nullptr)
	{
		return false;
	}

	const TileType targetTileType = ctx.map->get_tile_type(targetPosition);

	// The step is the same one every monster takes; what is said and what it costs are
	// the player's own.
	switch (Pathfinding::step(playerOwner, targetPosition, ctx))
	{

	case Pathfinding::StepResult::MOVED:
	{
		ctx.map->describe_tile(targetTileType, ctx);
		spend_player_action(ctx, TIME_UNITS_PER_ROUND);
		shouldComputeFOV = true;
		return true;
	}

	case Pathfinding::StepResult::OPENED_DOOR:
	{
		spend_player_action(ctx, TIME_UNITS_PER_ROUND);
		return false;
	}

	case Pathfinding::StepResult::STOPPED:
	{
		return false;
	}

	case Pathfinding::StepResult::REFUSED:
	{
		break;
	}
	}

	switch (targetTileType)
	{

	case TileType::WATER:
	case TileType::WALL:
	{
		// What a blocked tile says is authored in tile_config.json.
		const std::string& blockedMessage =
			ctx.tileConfig->get_tile_definition(targetTileType).blockedMessage;
		if (!blockedMessage.empty())
		{
			ctx.messageSystem->log(blockedMessage);
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, blockedMessage, MessageCompletion::FINISHED);
		}
		break;
	}

	case TileType::CLOSED_DOOR:
	{
		// An unlocked door opened in the step; a refused one is locked.
		if (ctx.map->is_door_locked(targetPosition))
		{
			resolve_locked_door(targetPosition, ctx);
		}
		break;
	}

	default:
	{
		break;
	}
	}
	return false;
}

bool PlayerController::resolve_mouse_world_tile(GameContext& ctx, Vector2D& out_world_tile) const
{
	assert(ctx.map && "PlayerController::resolve_mouse_world_tile: called without a map");
	assert(ctx.inputSystem && "PlayerController::resolve_mouse_world_tile: called without an inputSystem");
	if (!ctx.renderer)
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

void PlayerController::flush_fov(GameContext& ctx)
{
	assert(ctx.map && "PlayerController::flush_fov: called without a map");

	if (shouldComputeFOV)
	{
		shouldComputeFOV = false;
		ctx.map->compute_fov(ctx);
	}
}

bool PlayerController::is_mouse_pending_cancelled(GameContext& ctx) const
{
	assert(ctx.inputHandler && "PlayerController::is_mouse_pending_cancelled: called without an inputHandler");

	const Controls key = static_cast<Controls>(ctx.inputHandler->get_current_key());
	const auto& moves = direction_map();
	bool isMovement = moves.contains(key);
	bool isAction = (key == Controls::ESCAPE || key == Controls::WAIT || key == Controls::PICK);
	return isMovement || isAction;
}

Vector2D PlayerController::find_door_approach(Vector2D doorTile, const GameContext& ctx) const
{
	assert(ctx.map && "PlayerController::find_door_approach: called without a map");

	const std::array<Vector2D, 4> dirs{
		Vector2D{ 0, -1 },
		Vector2D{ 0, 1 },
		Vector2D{ -1, 0 },
		Vector2D{ 1, 0 },
	};
	Vector2D best{ -1, -1 };
	int bestDist = INT_MAX;
	for (auto direction : dirs)
	{
		Vector2D adj{ doorTile.x + direction.x, doorTile.y + direction.y };
		if (!Pathfinding::can_enter(playerOwner, adj, ctx))
		{
			continue;
		}

		int dist = std::abs(adj.x - ctx.player()->position.x) + std::abs(adj.y - ctx.player()->position.y);
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
	assert(ctx.map && "PlayerController::resolve_locked_door: called without a map");
	assert(ctx.dice && "PlayerController::resolve_locked_door: called without dice");

	// Branch 1: player carries a dungeon key -- consume it, unlock, open.
	Item* keyItem = nullptr;
	for (const auto& item : playerOwner.inventoryData.items)
	{
		assert(item && "PlayerController::resolve_locked_door: the pack holds a null item");
		if (item->itemKey == "dungeon_key")
		{
			keyItem = item.get();
			break;
		}
	}

	if (keyItem != nullptr)
	{
		[[maybe_unused]] const auto consumeUnlockResult = InventoryOperations::remove_item(playerOwner.inventoryData, *keyItem);
		assert(consumeUnlockResult.has_value() && "PlayerController::resolve_locked_door: the key found in the pack could not be taken from it");
		ctx.map->open_all_room_doors(doorPos, ctx);
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You use the key. The lock turns.", MessageCompletion::FINISHED);
		spend_player_action(ctx, TIME_UNITS_PER_ROUND);
		return true;
	}

	// Branch 2: Rogue -- Open Locks percentage roll (PHB Tables 26 to 29), once per lock per
	// level. A score of zero, or armour the book gives no column, means no skill.
	const int openLocksChance = playerOwner.thief_skill(ThiefSkill::OPEN_LOCKS).value_or(0);
	if (openLocksChance > 0)
	{
		const int level = playerOwner.get_creature_level();
		if (level > ctx.map->lock_pick_failed_at_level(doorPos))
		{
			// The 1d10 rounds are rolled before the d100, so a failure pays them as a success does.
			const int rounds = ctx.dice->d10();
			if (ctx.dice->d100() <= openLocksChance)
			{
				ctx.map->unlock_door(doorPos);
				ctx.map->open_door(doorPos, ctx);
				ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You pick the lock.", MessageCompletion::FINISHED);
			}
			else
			{
				ctx.map->mark_lock_pick_failed(doorPos, level);
				ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You fail to pick the lock.", MessageCompletion::FINISHED);
			}
			spend_player_action(ctx, rounds * TIME_UNITS_PER_ROUND);
			return true;
		}

		// Refused without a roll or a round, and the bump goes on to forcing.
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "This lock is too difficult for you until you have learned more.", MessageCompletion::FINISHED);
	}

	// Branch 3: force it, once per door, by Table 1's parenthesised Open Doors figure.
	if (ctx.map->has_resisted_forcing(doorPos))
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You have already failed to force this door.", MessageCompletion::FINISHED);
		return false;
	}

	assert(ctx.dataManager && "PlayerController::resolve_locked_door: called without a dataManager");
	const StrengthAttributes strengthRow = ctx.dataManager->strength_for(playerOwner.get_strength(), playerOwner.get_exceptional_strength());
	const int lockedDoorChance = strengthRow.lockedDoors;
	if (lockedDoorChance > 0)
	{
		if (ctx.dice->d20() <= lockedDoorChance)
		{
			ctx.map->unlock_door(doorPos);
			ctx.map->open_door(doorPos, ctx);
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You force the door open! The crash echoes down the corridor.", MessageCompletion::FINISHED);
		}
		else
		{
			ctx.map->mark_resisted_forcing(doorPos);
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You slam into the door but it holds.", MessageCompletion::FINISHED);
		}
		spend_player_action(ctx, TIME_UNITS_PER_ROUND);
		return true;
	}

	// Branch 4: a Strength row that prints no locked-door chance.
	ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "The door is locked. You are not strong enough to force it.", MessageCompletion::FINISHED);
	return false;
}

void PlayerController::begin_path_walk(
	Vector2D walkDest,
	Vector2D actionTarget,
	MouseMode mode,
	PendingDoorAction doorAction,
	GameContext& ctx)
{
	assert(ctx.mousePathOverlay && "PlayerController::begin_path_walk: called without a mousePathOverlay");

	if (mode == MouseMode::IDLE)
	{
		throw std::logic_error("PlayerController::begin_path_walk -- mode must not be IDLE");
	}

	assert(ctx.map && "PlayerController::begin_path_walk: called without a map");
	assert(ctx.pathfinder && "PlayerController::begin_path_walk: called without a pathfinder");

	assert(ctx.tileConfig && "begin_path_walk: a route was planned with no tile configuration");
	auto path = ctx.pathfinder->find_route(
		ctx.player()->position,
		walkDest,
		Pathfinding::mobility_of(playerOwner, *ctx.tileConfig),
		ctx);

	if (path.empty())
	{
		return;
	}

	*ctx.mousePathOverlay = std::move(path);
	mouseMode = mode;
	mouseDoorAction = doorAction;
	mouseDoorTarget = actionTarget;
}

bool PlayerController::execute_arrival(GameContext& ctx)
{
	assert(ctx.levelManager && "PlayerController::execute_arrival: called without a levelManager");
	assert(ctx.map && "PlayerController::execute_arrival: called without a map");

	switch (mouseMode)
	{

	case MouseMode::WALK_TO_PICKUP:
	{
		pick_item(ctx);
		spend_player_action(ctx, TIME_UNITS_PER_ROUND);
		return true;
	}

	case MouseMode::WALK_TO_DOOR:
	{
		if (mouseDoorTarget.x == -1)
		{
			throw std::logic_error("PlayerController::execute_arrival -- WALK_TO_DOOR reached without a valid mouseDoorTarget");
		}

		if (mouseDoorAction == PendingDoorAction::OPEN)
		{
			ctx.map->open_door(mouseDoorTarget, ctx);
		}
		else
		{
			ctx.map->close_door(mouseDoorTarget, ctx);
		}

		spend_player_action(ctx, TIME_UNITS_PER_ROUND);
		return true;
	}

	case MouseMode::WALK_TO_STAIRS:
	{
		assert(ctx.stairs && "PlayerController::execute_arrival: called without stairs");
		if (ctx.stairs->position == playerOwner.position)
		{
			ctx.levelManager->advance_to_next_level(ctx);
			ctx.gameState->set_game_status(GameStatus::STARTUP);
		}
		return true;
	}

	default:
	{
		return false;
	}
	}
}

bool PlayerController::handle_mouse_path(GameContext& ctx)
{
	assert(ctx.mousePathOverlay && "PlayerController::handle_mouse_path: called without a mousePathOverlay");
	assert(ctx.map && "PlayerController::handle_mouse_path: called without a map");

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

	while (!ctx.mousePathOverlay->empty() && ctx.mousePathOverlay->front() == playerOwner.position)
	{
		ctx.mousePathOverlay->erase(ctx.mousePathOverlay->begin());
	}

	if (ctx.mousePathOverlay->empty())
	{
		bool turnConsumed = execute_arrival(ctx);
		mouseMode = MouseMode::IDLE;
		return turnConsumed;
	}

	Vector2D next = ctx.mousePathOverlay->front();
	Vector2D prevPos = playerOwner.position;
	const bool isShutDoorAhead = ctx.map->get_tile_type(next) == TileType::CLOSED_DOOR;
	look_to_move(next, ctx);
	look_to_attack(next, ctx);
	look_on_floor(next, ctx);
	flush_fov(ctx);

	if (playerOwner.position == next)
	{
		ctx.mousePathOverlay->erase(ctx.mousePathOverlay->begin());
	}

	// A door the route runs through is opened on one turn and walked on the next, so
	// standing still after opening it is progress rather than an obstacle.
	const bool isDoorOpenedOnTheWay = isShutDoorAhead && ctx.map->is_open_door(next);
	if (playerOwner.position == prevPos && !isDoorOpenedOnTheWay)
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
	{
		spend_player_action(ctx, TIME_UNITS_PER_ROUND);
	}

	return true;
}

void PlayerController::handle_left_click(GameContext& ctx)
{
	assert(ctx.levelManager && "PlayerController::handle_left_click: called without a levelManager");
	assert(ctx.floorInventory && "PlayerController::handle_left_click: called without a floorInventory");
	assert(ctx.map && "PlayerController::handle_left_click: called without a map");

	Vector2D world_tile;
	if (!resolve_mouse_world_tile(ctx, world_tile))
	{
		return;
	}

	if (world_tile == playerOwner.position)
	{
		assert(ctx.stairs && "PlayerController::handle_left_click: a click on the player came without stairs");
		if (ctx.stairs->position == playerOwner.position)
		{
			ctx.levelManager->advance_to_next_level(ctx);
			ctx.gameState->set_game_status(GameStatus::STARTUP);
		}
		else
		{
			pick_item(ctx);
			spend_player_action(ctx, TIME_UNITS_PER_ROUND);
		}
		return;
	}

	if (ctx.map->get_actor(world_tile, ctx) != nullptr)
	{
		int deltaCol = std::abs(playerOwner.position.x - world_tile.x);
		int deltaRow = std::abs(playerOwner.position.y - world_tile.y);
		if (deltaCol <= 1 && deltaRow <= 1)
		{
			look_to_attack(world_tile, ctx);
			flush_fov(ctx);
			spend_player_action(ctx, TIME_UNITS_PER_ROUND);
		}
		return;
	}

	MouseMode mode = MouseMode::WALK;
	assert(ctx.stairs && "PlayerController::handle_left_click: a click away from the player came without stairs");
	if (ctx.stairs->position == world_tile)
	{
		mode = MouseMode::WALK_TO_STAIRS;
	}
	else
	{
		for (const auto& item : ctx.floorInventory->items)
		{
			assert(item && "handle_left_click: the floor holds a null where an item should be");

			if (item->position == world_tile)
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
	assert(ctx.floorInventory && "PlayerController::handle_right_click: called without a floorInventory");
	assert(ctx.map && "PlayerController::handle_right_click: called without a map");

	Vector2D world_tile;
	if (!resolve_mouse_world_tile(ctx, world_tile))
	{
		return;
	}

	int tileSize = ctx.renderer->get_tile_size();
	// The screen column is the drawn position divided down, as the tile renderer does;
	// the camera is not tile-aligned, so dividing it separately is off by one.
	int anchor_col = (world_tile.x * tileSize - ctx.renderer->get_camera_x()) / tileSize;
	int anchor_row = (world_tile.y * tileSize - ctx.renderer->get_camera_y()) / tileSize;

	std::vector<ContextAction> actions;

	bool isPlayerTile = (world_tile == playerOwner.position);

	if (isPlayerTile)
	{
		const auto open_inventory = [this](GameContext& actionCtx)
		{
			display_inventory(actionCtx);
		};
		const auto open_character_sheet = [this](GameContext& actionCtx)
		{
			actionCtx.displayManager->display_character_sheet(playerOwner, actionCtx);
		};
		const auto rest_here = [this](GameContext& actionCtx)
		{
			playerOwner.rest(actionCtx);
		};
		actions.push_back({ "Open Inventory", open_inventory });
		actions.push_back({ "Character Sheet", open_character_sheet });
		actions.push_back({ "Rest", rest_here });
		if (!playerOwner.spellcasting.memorizedSpells.empty())
		{
			const auto open_casting_menu = [this](GameContext& actionCtx)
			{
				SpellSystem::show_casting_menu(playerOwner, actionCtx);
			};
			actions.push_back({ "Cast Spell", open_casting_menu });
		}
	}

	// Floor item at tile
	for (auto& item : ctx.floorInventory->items)
	{
		assert(item && "handle_right_click: the floor holds a null where an item should be");

		if (item->position == world_tile)
		{
			std::string itemName = item->get_name().substr(0, 16);
			const auto walk_to_pick_up = walk_to_pick_up_command(world_tile);
			actions.push_back({ "Pick up " + itemName, walk_to_pick_up });
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

		// Beside the door it is worked at once; further off, the player walks up to it first.
		const auto work_the_door = work_the_door_command(world_tile, doorAction);
		actions.push_back({ doorLabel, work_the_door });
	}

	// Stairs at tile
	assert(ctx.stairs && "PlayerController::handle_right_click: called without stairs");
	if (ctx.stairs->position == world_tile)
	{
		const auto walk_to_stairs = [this, world_tile](GameContext& actionCtx)
		{
			begin_path_walk(
				world_tile,
				world_tile,
				MouseMode::WALK_TO_STAIRS,
				PendingDoorAction::NONE,
				actionCtx);
		};
		actions.push_back({ "Descend stairs", walk_to_stairs });
	}

	// Monster at tile (non-player)
	Creature* creature = ctx.map->get_actor(world_tile, ctx);
	if (creature && !creature->is_player())
	{
		if (playerOwner.position.chebyshev_distance_to(world_tile) <= 1)
		{
			const std::string monsterName = creature->actorData.name.substr(0, 16);
			const auto attack_it = [this, world_tile](GameContext& actionCtx)
			{
				Vector2D target = world_tile;
				look_to_attack(target, actionCtx);
				flush_fov(actionCtx);
				spend_player_action(actionCtx, TIME_UNITS_PER_ROUND);
			};
			actions.push_back({ "Attack " + monsterName, attack_it });
		}
	}

	// Walk here -- any non-player walkable tile with no creature
	if (!isPlayerTile && Pathfinding::can_enter(playerOwner, world_tile, ctx) && creature == nullptr)
	{
		const auto walk_here = [this, world_tile](GameContext& actionCtx)
		{
			begin_path_walk(
				world_tile,
				world_tile,
				MouseMode::WALK,
				PendingDoorAction::NONE,
				actionCtx);
		};
		actions.push_back({ "Walk here", walk_here });
	}

	// Nothing actionable -- skip the menu entirely
	if (actions.empty())
	{
		return;
	}

	// Cancel closes the menu and does nothing else.
	const auto do_nothing = []([[maybe_unused]] GameContext& actionCtx) {
	};
	actions.push_back({ "Cancel", do_nothing });

	std::vector<std::string> labels;
	labels.reserve(actions.size());
	for (const auto& action : actions)
	{
		labels.push_back(action.label);
	}

	auto on_select = [actions = std::move(actions)](int selected, GameContext& selectCtx)
	{
		if (selected >= 0 && selected < static_cast<int>(actions.size()))
		{
			actions[selected].execute(selectCtx);
		}
	};

	ctx.menus->push_back(std::make_unique<ContextMenu>(
		std::move(labels), anchor_col, anchor_row, std::move(on_select), ctx));
}

std::function<void(GameContext&)> PlayerController::walk_to_pick_up_command(Vector2D worldTile)
{
	return [this, worldTile](GameContext& actionCtx)
	{
		begin_path_walk(
			worldTile,
			worldTile,
			MouseMode::WALK_TO_PICKUP,
			PendingDoorAction::NONE,
			actionCtx);
	};
}

std::function<void(GameContext&)> PlayerController::work_the_door_command(
	Vector2D worldTile,
	PendingDoorAction doorAction)
{
	return [this, worldTile, doorAction](GameContext& actionCtx)
	{
		if (actionCtx.player()->position.chebyshev_distance_to(worldTile) <= 1)
		{
			if (doorAction == PendingDoorAction::OPEN)
			{
				actionCtx.map->open_door(worldTile, actionCtx);
			}
			else
			{
				actionCtx.map->close_door(worldTile, actionCtx);
			}
			spend_player_action(actionCtx, TIME_UNITS_PER_ROUND);
		}
		else
		{
			const Vector2D approach = find_door_approach(worldTile, actionCtx);
			if (approach.x != -1)
			{
				begin_path_walk(approach, worldTile, MouseMode::WALK_TO_DOOR, doorAction, actionCtx);
			}
		}
	};
}

void PlayerController::call_action(Controls key, GameContext& ctx)
{
	assert(ctx.dataManager && "PlayerController::call_action: called without a dataManager");
	assert(ctx.costOverlay && "PlayerController::call_action: called without a costOverlay");
	assert(ctx.displayManager && "PlayerController::call_action: called without a displayManager");
	assert(ctx.stairs && "PlayerController::call_action: called without stairs");
	assert(ctx.targeting && "PlayerController::call_action: called without a targeting");
	assert(ctx.levelManager && "PlayerController::call_action: called without a levelManager");
	assert(ctx.creatureManager && "PlayerController::call_action: called without a creatureManager");
	assert(ctx.map && "PlayerController::call_action: called without a map");

	switch (key)
	{

	case Controls::WAIT:
	{
		spend_player_action(ctx, TIME_UNITS_PER_ROUND);
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
		spend_player_action(ctx, TIME_UNITS_PER_ROUND);
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
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You quit the game ! Press any key ...", MessageCompletion::FINISHED);
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

	case Controls::COST_OVERLAY:
	{
		ctx.costOverlay->toggle();
		break;
	}

	case Controls::TEST_COMMAND:
	{
		ItemFactory::spawn_all_enhanced_items_debug(playerOwner.position, ctx);
		// Weight-gated: an overloaded player is refused and told, since a debug key must not abort.
		const auto debugSpawnBowResult = InventoryOperations::add_item_to_inventory(
			playerOwner.inventoryData,
			ItemCreator::create("long_bow", playerOwner.position, ctx),
			playerOwner,
			*ctx.dataManager);
		if (debugSpawnBowResult.has_value())
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "DEBUG: Long bow added to inventory.", MessageCompletion::FINISHED);
		}
		else
		{
			// The reason comes from the error the add already produced, so the message
			// cannot drift from what actually refused it.
			const std::string refusal = (debugSpawnBowResult.error() == InventoryError::FULL)
				? "the pack is full"
				: "it is too heavy to carry";
			ctx.messageSystem->message(ColorPairId::WHITE_RED, "DEBUG: the long bow was refused - " + refusal + ".", MessageCompletion::FINISHED);
		}

		std::vector<std::string>& memorized = playerOwner.spellcasting.memorizedSpells;
		memorized.push_back("magic_missile");
		memorized.push_back("magic_missile");
		memorized.push_back("sleep");
		memorized.push_back("web");
		memorized.push_back("teleport");
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "DEBUG: Spells added -- press Shift+C to cast.", MessageCompletion::FINISHED);

		// Spawn a shopkeeper on the first walkable adjacent tile, and say so when all four are wall.
		const std::array<Vector2D, 4> cardinals{
			Vector2D{ 0, -1 },
			Vector2D{ 0, 1 },
			Vector2D{ -1, 0 },
			Vector2D{ 1, 0 },
		};
		bool shopkeeperSpawned = false;
		for (const auto& offset : cardinals)
		{
			Vector2D spawnPos = playerOwner.position + offset;
			if (ctx.map->can_place_at(spawnPos, ctx))
			{
				ctx.creatureManager->add_creature(MonsterCreator::create(spawnPos, MonsterId::SHOPKEEPER, ctx), ctx);
				ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "DEBUG: Shopkeeper spawned.", MessageCompletion::FINISHED);
				shopkeeperSpawned = true;
				break;
			}
		}
		if (!shopkeeperSpawned)
		{
			ctx.messageSystem->message(ColorPairId::WHITE_RED, "DEBUG: no walkable tile beside the player to spawn a shopkeeper on.", MessageCompletion::FINISHED);
		}
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
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Which direction? (use arrow keys)", MessageCompletion::FINISHED);
		pendingDoorAction = PendingDoorAction::OPEN;
		break;
	}

	case Controls::CLOSE_DOOR:
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Which direction? (use arrow keys)", MessageCompletion::FINISHED);
		pendingDoorAction = PendingDoorAction::CLOSE;
		break;
	}

	case Controls::DISARM:
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Which direction? (use arrow keys)", MessageCompletion::FINISHED);
		pendingDoorAction = PendingDoorAction::DISARM;
		break;
	}

	case Controls::SEARCH:
	{
		// A character who cannot look for traps is told so and loses no time; a search
		// costs every round it took.
		const std::optional<int> rounds = TrapSearch::search_around(playerOwner, ctx);
		if (rounds.has_value())
		{
			spend_player_action(ctx, rounds.value() * TIME_UNITS_PER_ROUND);
		}
		break;
	}

	case Controls::REST:
	{
		playerOwner.rest(ctx);
		break;
	}

	case Controls::TURN_UNDEAD:
	{
		attempt_turn_undead(ctx);
		break;
	}

	case Controls::HIDE:
	{
		// The turn is spent on an attempt, not on a success: the thief does not
		// know which it was, and a free turn would tell him.
		if (playerOwner.attempt_hide(ctx) == Player::HideAttempt::ATTEMPTED)
		{
			spend_player_action(ctx, TIME_UNITS_PER_ROUND);
		}
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
	{
		break;
	}
	}
}

bool PlayerController::resolve_pending_door(GameContext& ctx)
{
	assert(ctx.inputHandler && "PlayerController::resolve_pending_door: called without an inputHandler");
	assert(ctx.map && "PlayerController::resolve_pending_door: called without a map");

	if (pendingDoorAction == PendingDoorAction::NONE)
	{
		return false;
	}

	int dirKey = ctx.inputHandler->get_current_key();
	if (dirKey == 27)
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Cancelled.", MessageCompletion::FINISHED);
		pendingDoorAction = PendingDoorAction::NONE;
		return true;
	}
	if (dirKey == -1)
	{
		return true;
	}

	Vector2D doorPos = handle_direction_input(dirKey, ctx);
	if (doorPos.x == 0 && doorPos.y == 0)
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Invalid direction.", MessageCompletion::FINISHED);
		pendingDoorAction = PendingDoorAction::NONE;
		return true;
	}

	// A trap sits on floor, never on a door, so a disarm is answered before the door
	// check rather than refused by it.
	if (pendingDoorAction == PendingDoorAction::DISARM)
	{
		const std::optional<int> rounds = TrapDisarm::disarm_at(playerOwner, doorPos, ctx);
		if (rounds.has_value())
		{
			spend_player_action(ctx, rounds.value() * TIME_UNITS_PER_ROUND);
		}
		pendingDoorAction = PendingDoorAction::NONE;
		return true;
	}

	if (!ctx.map->is_door(doorPos))
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "There is no door there.", MessageCompletion::FINISHED);
		pendingDoorAction = PendingDoorAction::NONE;
		return true;
	}

	if (pendingDoorAction == PendingDoorAction::OPEN)
	{
		if (!ctx.map->is_door_locked(doorPos))
		{
			if (ctx.map->open_door(doorPos, ctx))
			{
				ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You open the door.", MessageCompletion::FINISHED);
				spend_player_action(ctx, TIME_UNITS_PER_ROUND);
			}
			else
			{
				ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "The door is already open.", MessageCompletion::FINISHED);
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
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You close the door.", MessageCompletion::FINISHED);
			spend_player_action(ctx, TIME_UNITS_PER_ROUND);
		}
		else if (ctx.map->get_actor(doorPos, ctx) != nullptr)
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Something is blocking the door.", MessageCompletion::FINISHED);
		}
		else
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "The door is already closed.", MessageCompletion::FINISHED);
		}
	}

	pendingDoorAction = PendingDoorAction::NONE;
	return true;
}

Vector2D PlayerController::handle_direction_input(int dirKey, GameContext& ctx)
{
	assert(ctx.map && "PlayerController::handle_direction_input: called without a map");

	const auto& moves = direction_map();
	const auto controlKey = static_cast<Controls>(dirKey);
	if (!moves.contains(controlKey))
	{
		return { 0, 0 };
	}
	const Vector2D targetPos = playerOwner.position + moves.at(controlKey);
	if (!ctx.map->is_in_bounds(targetPos))
	{
		return { 0, 0 };
	}

	return targetPos;
}

// end of file: PlayerController.cpp
