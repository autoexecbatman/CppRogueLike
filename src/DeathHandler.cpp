// file: DeathHandler.cpp
#include <cassert>
#include <format>
#include <memory>
#include <ranges>

#include "AnimationSystem.h"
#include "Colors.h"
#include "Creature.h"
#include "DeathHandler.h"
#include "GameContext.h"
#include "GameStateManager.h"
#include "InventoryOperations.h"
#include "Item.h"
#include "MessageSystem.h"
#include "Pickable.h"
#include "Player.h"
#include "TileConfig.h"

//==MonsterDeathHandler==

DestructibleType MonsterDeathHandler::type() const
{
	return DestructibleType::MONSTER;
}

void MonsterDeathHandler::execute(Creature& owner, GameContext& ctx)
{
	ctx.messageSystem->append_message_part(owner.actorData.color, std::format("{}", owner.actorData.name));
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " is dead.\n");
	ctx.messageSystem->finalize_message();

	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "You get ");
	ctx.messageSystem->append_message_part(ColorPairId::YELLOW_BLACK, std::format("{}", owner.get_xp()));
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " experience points.\n");
	ctx.messageSystem->finalize_message();

	assert(ctx.player() != nullptr && "MonsterDeathHandler::execute requires a live player in context");
	ctx.player_concrete().on_kill_reward(owner.get_xp(), ctx);

	if (ctx.animSystem)
	{
		ctx.animSystem->spawn_death(owner.position);
	}

	[[maybe_unused]] auto is_nothing = [](const auto& carried)
	{
		return !carried;
	};
	assert(std::ranges::none_of(owner.inventoryData.items, is_nothing) && "a pack holds nothing where an item should be");
	for (auto& item : owner.inventoryData.items)
	{
		item->position = owner.position;
		[[maybe_unused]] const auto dropOnDeathResult = InventoryOperations::add_item(*ctx.floorInventory, std::move(item));
		assert(dropOnDeathResult.has_value());
	}
	owner.inventoryData.items.clear();

	// Create a corpse on the floor in place of the creature.
	auto corpse = std::make_unique<Item>(owner.position, owner.actorData);
	corpse->actorData.name = std::format("dead {}", owner.get_name());
	corpse->actorData.tile = ctx.tileConfig->get("TILE_CORPSE");
	corpse->enhancement.weight = owner.get_corpse_weight();
	corpse->behavior = CorpseFood{ 0 };
	[[maybe_unused]] const auto placeCorpseResult = InventoryOperations::add_item(*ctx.floorInventory, std::move(corpse));
	assert(placeCorpseResult.has_value());
}

//==PlayerDeathHandler==

DestructibleType PlayerDeathHandler::type() const
{
	return DestructibleType::PLAYER;
}

void PlayerDeathHandler::execute(Creature& owner, GameContext& ctx)
{
	ctx.gameState->set_game_status(GameStatus::DEFEAT);
	[[maybe_unused]] const bool deleted = ctx.stateManager->delete_save_file();
}

// end of file: DeathHandler.cpp
