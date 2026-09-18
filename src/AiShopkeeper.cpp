#include <cassert>
#include <memory>

#include "Creature.h"
#include "GameContext.h"
#include "MenuTrade.h"
#include "Persistent.h"
#include "MenuManager.h"
#include "Ai.h"
#include "AiShopkeeper.h"


void AiShopkeeper::update(Creature& owner, GameContext& ctx)
{
	assert(owner.ai != nullptr && "AiShopkeeper::update called on creature with null ai");

	if (owner.is_dead())
	{
		return;
	}

	// TODO: implement shopkeeper idle behavior (tend shop, wander near spawn point)
}

void AiShopkeeper::load(const json& j)
{
}

void AiShopkeeper::save(json& j)
{
	j["type"] = static_cast<int>(AiType::SHOPKEEPER);
}
