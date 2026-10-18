#include <cassert>
#include <memory>

#include "Ai.h"
#include "AiShopkeeper.h"
#include "Creature.h"
#include "GameContext.h"
#include "LevelManager.h"
#include "Persistent.h"
#include "ShopkeeperFactory.h"

void AiShopkeeper::update(Creature& owner, [[maybe_unused]] GameContext& ctx)
{
	assert(owner.ai != nullptr && "AiShopkeeper::update called on creature with null ai");

	if (owner.is_dead())
	{
		return;
	}

	// TODO: implement shopkeeper idle behavior (tend shop, wander near spawn point)
}

void AiShopkeeper::on_created(Creature& owner, GameContext& ctx)
{
	assert(ctx.levelManager && "AiShopkeeper::on_created: called without a levelManager");
	ShopkeeperFactory::open_shop(owner, ctx.levelManager->get_dungeon_level(), ctx);
}

void AiShopkeeper::load([[maybe_unused]] const json& j)
{
}

void AiShopkeeper::save(json& j)
{
	j["type"] = encode_ai_type(get_ai_type());
}
