// file: DeathHandler.cpp
#include <cassert>
#include <format>
#include <memory>
#include <ranges>

#include "Creature.h"
#include "InventoryOperations.h"
#include "Item.h"
#include "Pickable.h"
#include "Colors.h"
#include "GameContext.h"
#include "AnimationSystem.h"
#include "GameStateManager.h"
#include "MessageSystem.h"
#include "TileConfig.h"
#include "DeathHandler.h"
#include "Player.h"

//==MonsterDeathHandler==

DestructibleType MonsterDeathHandler::type() const
{
    return DestructibleType::MONSTER;
}

void MonsterDeathHandler::execute(Creature& owner, GameContext& ctx)
{
    ctx.messageSystem->append_message_part(owner.actorData.color, std::format("{}", owner.actorData.name));
    ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " is dead.\n");
    ctx.messageSystem->finalize_message();

    ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You get ");
    ctx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, std::format("{}", owner.get_xp()));
    ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " experience points.\n");
    ctx.messageSystem->finalize_message();

    assert(ctx.player() != nullptr && "MonsterDeathHandler::execute requires a live player in context");
    ctx.player_concrete().on_kill_reward(owner.get_xp(), ctx);

    if (ctx.animSystem)
    {
        ctx.animSystem->spawn_death(owner.position);
    }

    assert(std::ranges::none_of(owner.inventoryData.items, [](const auto& i) { return !i; }));
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
