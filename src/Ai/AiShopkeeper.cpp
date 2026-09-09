#include <cassert>
#include <memory>

#include "../Actor/Creature.h"
#include "../Core/GameContext.h"
#include "../Menu/MenuTrade.h"
#include "../Persistent/Persistent.h"
#include "../Systems/MenuManager.h"
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
