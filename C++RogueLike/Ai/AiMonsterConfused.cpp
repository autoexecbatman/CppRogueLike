#include <iostream>
#include <memory>
#include <gsl/gsl>

#include <libtcod.h>

#include "../Game.h"
#include "../Actor/Actor.h"
#include "Ai.h"
#include "AiMonsterConfused.h"

//==ConfusedMonsterAi==
AiMonsterConfused::AiMonsterConfused(int nbTurns, std::unique_ptr<Ai> oldAi) noexcept : nbTurns(nbTurns), oldAi(std::move(oldAi)) {}

void AiMonsterConfused::update(Actor& owner)
{
	const gsl::not_null<TCODRandom*> rng = TCODRandom::getInstance();
	Vector2D direction{ rng->getInt(-1, 1), rng->getInt(-1, 1) };

	if (direction != Vector2D{0, 0})
	{
		Vector2D destination = owner.position + direction;

		if (game.map->can_walk(destination))
		{
			owner.position = destination;
		}
		else
		{
			const auto& actor = game.get_actor(destination);
			if (actor)
			{
				owner.attacker->attack(owner, *actor);
			}
		}
	}

	nbTurns--;
	if (nbTurns == 0)
	{
		owner.ai = std::move(oldAi);
	}
}

void AiMonsterConfused::load(TCODZip& zip)
{
	nbTurns = zip.getInt();
	oldAi = Ai::create(zip);
}

void AiMonsterConfused::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<AiType>>(AiType::CONFUSED_MONSTER));
	zip.putInt(nbTurns);
	if (oldAi != nullptr)
	{
		oldAi->save(zip);
	}
	else
	{
		std::cout << "Error: save() called on actor with no oldAi." << std::endl;
		exit(-1);
	}
}