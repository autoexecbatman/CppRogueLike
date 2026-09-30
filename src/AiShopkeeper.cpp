#include <cassert>
#include <memory>

#include "Ai.h"
#include "AiShopkeeper.h"
#include "Creature.h"
#include "GameContext.h"
#include "MenuManager.h"
#include "MenuTrade.h"
#include "Persistent.h"

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
	j["type"] = encode_ai_type(get_ai_type());
}
