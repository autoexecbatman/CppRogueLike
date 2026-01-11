#include <iostream>
#include <memory>

#include <libtcod.h>

#include "../Game.h"
#include "../Actor/Actor.h"
#include "Ai.h"
#include "AiMonsterConfused.h"

//==ConfusedMonsterAi==
AiMonsterConfused::AiMonsterConfused(int nbTurns, std::unique_ptr<Ai> oldAi) noexcept : nbTurns(nbTurns), oldAi(std::move(oldAi)) {}

void AiMonsterConfused::update(Creature& owner, GameContext& ctx)
{
	TCODRandom* rng = TCODRandom::getInstance();
	Vector2D direction{ rng->getInt(-1, 1), rng->getInt(-1, 1) };

	if (direction != Vector2D{0, 0})
	{
		Vector2D destination = owner.position + direction;

		if (ctx.map->can_walk(destination, ctx))
		{
			owner.position = destination;
		}
		else
		{
			const auto& actor = ctx.map->get_actor(destination, ctx);
			if (actor)
			{
				owner.attacker->attack(owner, *actor, ctx);
			}
		}
	}

	nbTurns--;
	if (nbTurns == 0)
	{
		owner.ai = std::move(oldAi);
	}
}

void AiMonsterConfused::load(const json& j)
{
	nbTurns = j.at("nbTurns").get<int>();

	// Create the oldAi if it exists in the JSON
	if (j.contains("oldAi")) {
		oldAi = Ai::create(j["oldAi"]);
	}
}

void AiMonsterConfused::save(json& j)
{
	j["type"] = static_cast<int>(AiType::CONFUSED_MONSTER);
	j["nbTurns"] = nbTurns;

	// Save the oldAi if it exists
	if (oldAi != nullptr) {
		json oldAiJson;
		oldAi->save(oldAiJson);
		j["oldAi"] = oldAiJson;
	}
}