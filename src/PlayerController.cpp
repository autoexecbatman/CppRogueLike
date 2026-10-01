// file: PlayerController.cpp
#include <array>
#include <cassert>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "Actor.h"
#include "AttackKind.h"
#include "Colors.h"
#include "ContextMenu.h"
#include "Controls.h"
#include "Creature.h"
#include "CreatureManager.h"
#include "DecorEditor.h"
#include "Decoration.h"
#include "Dijkstra.h"
#include "DisplayManager.h"
#include "GameContext.h"
#include "InputHandler.h"
#include "InputSystem.h"
#include "InventoryOperations.h"
#include "InventoryUI.h"
#include "Item.h"
#include "ItemClassification.h"
#include "ItemCreator.h"
#include "ItemFactory.h"
#include "LevelManager.h"
#include "ListMenu.h"
#include "Map.h"
#include "Menu.h"
#include "MenuEntry.h"
#include "MenuTrade.h"
#include "MessageSystem.h"
#include "Persistent.h"
#include "Pickable.h"
#include "Player.h"
#include "PlayerController.h"
#include "Renderer.h"
#include "ShopkeeperFactory.h"
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
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
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

	// Handle confused state -- randomly move or act
	if (playerOwner.has_state(ActorState::IS_CONFUSED) && confusionTurns > 0)
	{
		confusionTurns--;

		if (confusionTurns == 0)
		{
			playerOwner.remove_state(ActorState::IS_CONFUSED);
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Your mind clears.", MessageCompletion::FINISHED);
		}
		else
		{
			if (ctx.dice->d2() == 1)
			{
				static const std::array<Vector2D, 8> allDirections = {
					DIR_N, DIR_S, DIR_W, DIR_E, DIR_NW, DIR_NE, DIR_SW, DIR_SE
				};
				moveVector = allDirections[ctx.dice->roll(0, 7)];

				ctx.messageSystem->message(ColorPairId::WHITE_GREEN, "You stumble around in confusion!", MessageCompletion::FINISHED);
				ctx.gameState->set_game_status(GameStatus::NEW_TURN);
			}
			else
			{
				ctx.messageSystem->message(ColorPairId::WHITE_GREEN, "You struggle to control your movements...", MessageCompletion::FINISHED);

				const auto& moves = direction_map();
				if (moves.contains(key))
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
		if (moves.contains(key))
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

// The controller holds only input state, which is rebuilt each frame. Player
// data belongs to Player and is written there.
void PlayerController::load(const json& savedState)
{
}

void PlayerController::save(json& savedState)
{
}

void PlayerController::move(Vector2D target)
{
	playerOwner.position = target;
}

void PlayerController::pick_item(GameContext& ctx)
{
	if (ctx.floorInventory->items.empty())
	{
		return;
	}

	// Find the first item at the player's position
	Item* item = nullptr;
	for (auto& floorItem : ctx.floorInventory->items)
	{
		assert(floorItem && "pick_item: the floor holds a null where an item should be");

		if (floorItem->position == playerOwner.position)
		{
			item = floorItem.get();
			break;
		}
	}

	if (!item)
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "There's nothing here to pick up.", MessageCompletion::FINISHED);
		return;
	}

	if (item->itemClass == ItemClass::GOLD_COIN)
	{
		Gold& goldBehavior = std::get<Gold>(*item->behavior);
		playerOwner.adjust_gold(goldBehavior.amount);
		ctx.messageSystem->message(ColorPairId::YELLOW_BLACK, "You picked up " + std::to_string(goldBehavior.amount) + " gold.", MessageCompletion::FINISHED);
		[[maybe_unused]] const auto takeGoldResult = InventoryOperations::remove_item(*ctx.floorInventory, *item);
		assert(takeGoldResult.has_value());
		return;
	}

	// Pre-check slot capacity before touching ownership
	if (InventoryOperations::is_inventory_full(playerOwner.inventoryData))
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Your inventory is full!", MessageCompletion::FINISHED);
		return;
	}

	// Pre-check weight before touching ownership — prevents item destruction on rejection
	if (!InventoryOperations::is_within_weight_limit(*item, playerOwner, *ctx.dataManager))
	{
		ctx.messageSystem->message(ColorPairId::RED_BLACK, "Too heavy to carry.", MessageCompletion::FINISHED);
		return;
	}

	const std::string itemName = item->actorData.name;

	// Remove from floor: ownership transfers to removeResult
	auto removeResult = InventoryOperations::remove_item(*ctx.floorInventory, *item);
	if (!removeResult.has_value())
	{
		ctx.messageSystem->log("ERROR: pick_item -- remove_item failed unexpectedly");
		return;
	}

	// Pre-checks passed; add cannot fail on capacity or weight
	auto addResult = InventoryOperations::add_item_to_inventory(
		playerOwner.inventoryData,
		std::move(*removeResult),
		playerOwner,
		*ctx.dataManager);

	if (addResult.has_value())
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You picked up the " + itemName + ".", MessageCompletion::FINISHED);
	}
	else
	{
		ctx.messageSystem->log("ERROR: pick_item -- add failed after pre-checks; item lost");
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
	if (ctx.floorInventory->items.empty())
	{
		return;
	}

	for (const auto& floorItem : ctx.floorInventory->items)
	{
		assert(floorItem && "look_on_floor: the floor holds a null where an item should be");

		if (floorItem->position == target)
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "There's a " + floorItem->actorData.name + " here", MessageCompletion::FINISHED);
		}
	}
}

bool PlayerController::look_to_attack(Vector2D& target, GameContext& ctx)
{
	for (const auto& c : *ctx.creatures)
	{
		assert(c != nullptr && "null creature in creatures list");
		if (!c->is_dead() && c->position == target)
		{
			assert(c->ai != nullptr && "living creature at target has no AI");

			// Bumping anything the player would not attack outright resolves as
			// an interaction rather than a swing.
			if (needs_attack_confirmation(c->get_attitude()))
			{
				resolve_peaceful_bump(*c, ctx);
				return false;
			}

			strike(*c, ctx);
			return false;
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
				{
					ctx.decorEditor->erase(decor->position);
				}

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

	ctx.gameState->set_game_status(GameStatus::NEW_TURN);
}

// Channels the player's deity against nearby undead and narrates the result.
//
// The system decides what happened; this decides what the player is told, and
// spends the turn only when an attempt was actually made.
void PlayerController::attempt_turn_undead(GameContext& ctx)
{
	const TurnUndeadReport report = turn_undead(playerOwner, ctx);

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
	ctx.gameState->set_game_status(GameStatus::NEW_TURN);
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
	playerOwner.roundCounter++;

	int attacksThisRound = 1;

	if (playerOwner.get_attacks_per_round() >= 2.0f)
	{
		attacksThisRound = 2;
	}
	else if (playerOwner.get_attacks_per_round() >= 1.5f)
	{
		// A 1.5 rate alternates: two swings on odd rounds, one on even.
		attacksThisRound = (playerOwner.roundCounter % 2 == 1) ? 2 : 1;
	}

	for (int attackIndex = 0; attackIndex < attacksThisRound; ++attackIndex)
	{
		if (target.is_dead())
		{
			break;
		}

		if (attackIndex > 0)
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Follow-up attack: ", MessageCompletion::FINISHED);
		}

		playerOwner.attacker->attack(target, AttackKind::MELEE, ctx);
	}

	ctx.creatureManager->cleanup_dead_creatures(*ctx.creatures);
}

bool PlayerController::look_to_move(const Vector2D& targetPosition, GameContext& ctx)
{
	if (ctx.map->get_actor(targetPosition, ctx) != nullptr)
	{
		return false;
	}

	TileType targetTileType = ctx.map->get_tile_type(targetPosition);

	if (!ctx.map->is_collision(playerOwner, targetTileType, targetPosition, ctx))
	{
		bool blocked = false;

		// Everything standing on the tile gets one entry, whichever container it
		// lives in. A feature that destroyed itself this turn is inert until the
		// sweep.
		const auto enter_all = [&](auto& features)
		{
			for (const auto& feature : features)
			{
				assert(feature && "look_to_move: a floor container holds a null entry");

				if (blocked || feature->is_destroyed() || feature->position != targetPosition)
				{
					continue;
				}

				if (feature->on_creature_enter(playerOwner, ctx) == EntryResult::BLOCKED)
				{
					blocked = true;
				}
			}
		};
		enter_all(*ctx.traps);
		enter_all(*ctx.spellTiles);

		if (!blocked)
		{
			move(targetPosition);
			ctx.map->describe_tile(targetTileType, ctx);
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
	bool isMovement = moves.contains(key);
	bool isAction = (key == Controls::ESCAPE || key == Controls::WAIT || key == Controls::PICK);
	return isMovement || isAction;
}

Vector2D PlayerController::find_door_approach(Vector2D doorTile, const GameContext& ctx) const
{
	if (!ctx.map)
	{
		throw std::logic_error("PlayerController::find_door_approach -- ctx.map is null");
	}

	const std::array<Vector2D, 4> dirs{
		Vector2D{ 0, -1 }, Vector2D{ 0, 1 }, Vector2D{ -1, 0 }, Vector2D{ 1, 0 }
	};
	Vector2D best{ -1, -1 };
	int bestDist = INT_MAX;
	for (auto d : dirs)
	{
		Vector2D adj{ doorTile.x + d.x, doorTile.y + d.y };
		if (!ctx.map->can_walk(adj, ctx))
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
	// Branch 1: player carries a dungeon key -- consume it, unlock, open.
	Item* keyItem = nullptr;
	for (const auto& item : playerOwner.inventoryData.items)
	{
		assert(item);
		if (item->itemKey == "dungeon_key")
		{
			keyItem = item.get();
			break;
		}
	}

	if (keyItem != nullptr)
	{
		[[maybe_unused]] const auto consumeUnlockResult = InventoryOperations::remove_item(playerOwner.inventoryData, *keyItem);
		assert(consumeUnlockResult.has_value());
		ctx.map->open_all_room_doors(doorPos, ctx);
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You use the key. The lock turns.", MessageCompletion::FINISHED);
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		return true;
	}

	// Branch 2: Rogue -- Open Locks percentage roll (PHB Tables 26 to 29). A score
	// of zero is a skill the thief has not bought up to a usable percentage yet,
	// and armour the book gives no column for answers with no skill at all.
	const int openLocksChance = playerOwner.thief_skill(ThiefSkill::OPEN_LOCKS).value_or(0);
	if (openLocksChance > 0)
	{
		if (ctx.dice->d100() <= openLocksChance)
		{
			ctx.map->unlock_door(doorPos);
			ctx.map->open_door(doorPos, ctx);
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You pick the lock.", MessageCompletion::FINISHED);
		}
		else
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You fail to pick the lock.", MessageCompletion::FINISHED);
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
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You smash the door open! The crash echoes down the corridor.", MessageCompletion::FINISHED);
		}
		else
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You slam into the door but it holds.", MessageCompletion::FINISHED);
		}
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		return true;
	}

	// Branch 4: no tool available.
	ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "The door is locked. You have no way through it.", MessageCompletion::FINISHED);
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
	{
		throw std::logic_error("PlayerController::begin_path_walk -- mode must not be IDLE");
	}

	if (!ctx.pathfinder || !ctx.map)
	{
		return;
	}

	auto path = ctx.pathfinder->a_star_search(*ctx.map, ctx.player()->position, walkDest, true, ctx);

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
	look_to_move(next, ctx);
	look_to_attack(next, ctx);
	look_on_floor(next, ctx);
	flush_fov(ctx);

	if (playerOwner.position == next)
	{
		ctx.mousePathOverlay->erase(ctx.mousePathOverlay->begin());
	}

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
	{
		ctx.gameState->set_game_status(GameStatus::NEW_TURN);
	}

	return true;
}

void PlayerController::handle_left_click(GameContext& ctx)
{
	Vector2D world_tile;
	if (!resolve_mouse_world_tile(ctx, world_tile))
	{
		return;
	}

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
	Vector2D world_tile;
	if (!resolve_mouse_world_tile(ctx, world_tile))
	{
		return;
	}

	int tileSize = ctx.renderer->get_tile_size();
	// Where that world tile actually sits on screen. The camera is a pixel value and
	// is not tile-aligned - set_camera_center subtracts half a viewport, which lands
	// on a half tile whenever the viewport is an odd number of columns wide. So the
	// screen column is the drawn position divided down, the same formula the tile
	// rendering uses; subtracting a separately-divided camera is off by one there.
	int anchor_col = (world_tile.x * tileSize - ctx.renderer->get_camera_x()) / tileSize;
	int anchor_row = (world_tile.y * tileSize - ctx.renderer->get_camera_y()) / tileSize;

	std::vector<ContextAction> actions;

	bool isPlayerTile = (world_tile == playerOwner.position);

	if (isPlayerTile)
	{
		actions.push_back({ "Open Inventory",
			[this](GameContext& c)
			{ display_inventory(c); } });
		actions.push_back({ "Character Sheet",
			[this](GameContext& c)
			{ c.displayManager->display_character_sheet(playerOwner, c); } });
		actions.push_back({ "Rest",
			[this](GameContext& c)
			{
				playerOwner.rest(c);
			} });
		if (!playerOwner.memorizedSpells.empty())
		{
			actions.push_back({ "Cast Spell",
				[this](GameContext& c)
				{ SpellSystem::show_casting_menu(playerOwner, c); } });
		}
	}

	// Floor item at tile
	for (auto& item : ctx.floorInventory->items)
	{
		assert(item && "handle_right_click: the floor holds a null where an item should be");

		if (item->position == world_tile)
		{
			std::string itemName = item->actorData.name.substr(0, 16);
			actions.push_back({ "Pick up " + itemName,
				[this, world_tile](GameContext& c)
				{
					begin_path_walk(
						world_tile,
						world_tile,
						MouseMode::WALK_TO_PICKUP,
						PendingDoorAction::NONE,
						c);
				} });
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

		actions.push_back({ doorLabel,
			[this, world_tile, doorAction](GameContext& c)
			{
				int dx = std::abs(c.player()->position.x - world_tile.x);
				int dy = std::abs(c.player()->position.y - world_tile.y);
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
			} });
	}

	// Stairs at tile
	if (ctx.stairs && ctx.stairs->position == world_tile)
	{
		actions.push_back({ "Descend stairs",
			[this, world_tile](GameContext& c)
			{
				begin_path_walk(
					world_tile,
					world_tile,
					MouseMode::WALK_TO_STAIRS,
					PendingDoorAction::NONE,
					c);
			} });
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
			actions.push_back({ "Attack " + monName,
				[this, world_tile](GameContext& c)
				{
					Vector2D target = world_tile;
					look_to_attack(target, c);
					flush_fov(c);
					c.gameState->set_game_status(GameStatus::NEW_TURN);
				} });
		}
	}

	// Walk here -- any non-player walkable tile with no creature
	if (!isPlayerTile && ctx.map->can_walk(world_tile, ctx) && creature == nullptr)
	{
		actions.push_back({ "Walk here",
			[this, world_tile](GameContext& c)
			{
				begin_path_walk(
					world_tile,
					world_tile,
					MouseMode::WALK,
					PendingDoorAction::NONE,
					c);
			} });
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

	case Controls::TEST_COMMAND:
	{
		ItemFactory::spawn_all_enhanced_items_debug(playerOwner.position, ctx);
		// Weight-gated, so an overloaded player is refused. That is a runtime outcome
		// rather than a wiring fault, which is why it is reported rather than asserted:
		// a debug command that kills the process because the player is carrying too
		// much is not a usable debug command.
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

		playerOwner.memorizedSpells.push_back("magic_missile");
		playerOwner.memorizedSpells.push_back("magic_missile");
		playerOwner.memorizedSpells.push_back("sleep");
		playerOwner.memorizedSpells.push_back("web");
		playerOwner.memorizedSpells.push_back("teleport");
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "DEBUG: Spells added -- press Shift+C to cast.", MessageCompletion::FINISHED);

		// Spawn a shopkeeper on the first walkable adjacent tile. All four can be wall -
		// in a corridor end, or a one-tile alcove - and saying nothing then leaves the
		// key looking broken, which is the same fault the bow above had.
		const std::array<Vector2D, 4> cardinals{ Vector2D{ 0, -1 }, Vector2D{ 0, 1 }, Vector2D{ -1, 0 }, Vector2D{ 1, 0 } };
		bool shopkeeperSpawned = false;
		for (const auto& offset : cardinals)
		{
			Vector2D spawnPos = playerOwner.position + offset;
			if (ctx.map->can_walk(spawnPos, ctx))
			{
				ctx.creatures->push_back(ShopkeeperFactory::create_shopkeeper(spawnPos, ctx.levelManager->get_dungeon_level(), ctx));
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
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
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
		break;
	}
}

bool PlayerController::resolve_pending_door(GameContext& ctx)
{
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
				ctx.gameState->set_game_status(GameStatus::NEW_TURN);
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
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
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
	else if (pendingDoorAction == PendingDoorAction::DISARM)
	{
		// Only a trap disarms, so only traps are asked. NOT_DISARMABLE survives
		// as the answer when the tile holds none.
		DisarmResult disarmResult = DisarmResult::NOT_DISARMABLE;
		if (ctx.traps)
		{
			for (auto& trap : *ctx.traps)
			{
				assert(trap && "resolve_pending_door: traps list holds a null entry");
				if (trap->is_destroyed() || trap->position != doorPos)
				{
					continue;
				}

				disarmResult = trap->attempt_disarm(playerOwner, ctx);
				if (disarmResult != DisarmResult::NOT_DISARMABLE)
				{
					break;
				}
			}
		}

		switch (disarmResult)
		{
		case DisarmResult::NOT_DISARMABLE:
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "There is no trap there.", MessageCompletion::FINISHED);
			break;
		}
		case DisarmResult::NOT_VISIBLE:
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You don't see a trap there.", MessageCompletion::FINISHED);
			break;
		}
		case DisarmResult::ALREADY_DISARMED:
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "That trap is already disarmed.", MessageCompletion::FINISHED);
			break;
		}
		case DisarmResult::NO_SKILL:
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You cannot work a trap.", MessageCompletion::FINISHED);
			break;
		}
		case DisarmResult::BEYOND_SKILL:
		{
			// The attempt is spent until the next level, so the turn is spent too.
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "This trap is beyond you.", MessageCompletion::FINISHED);
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
			break;
		}
		case DisarmResult::DISARMED:
		{
			ctx.messageSystem->message(ColorPairId::GREEN_BLACK, "You successfully disarm the trap.", MessageCompletion::FINISHED);
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
			break;
		}
		case DisarmResult::TRIGGERED:
		{
			ctx.messageSystem->message(ColorPairId::RED_BLACK, "You trigger the trap while attempting to disarm it!", MessageCompletion::FINISHED);
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
			break;
		}
		}
	}

	pendingDoorAction = PendingDoorAction::NONE;
	return true;
}

Vector2D PlayerController::handle_direction_input(int dirKey, GameContext& ctx)
{
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
