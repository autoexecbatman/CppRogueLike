// file: Ai.cpp
#include <iostream>
#include <curses.h>
#include <gsl/util>
#include <gsl/pointers>

#include "main.h"
#include "Menu.h"
#include "Colors.h"
#include "AiMonster.h"
#include "AiPlayer.h"

//==AI==
std::shared_ptr<Ai> Ai::create(TCODZip& zip) 
{
	const AiType type = gsl::narrow_cast<AiType>(zip.getInt());
	std::shared_ptr<Ai> ai = nullptr;

	switch (type) 
	{

	case AiType::PLAYER:
	{
		ai = std::make_shared<AiPlayer>();
		break;
	}

	case AiType::MONSTER:
	{
		ai = std::make_shared<AiMonster>();
		break;
	}
	
	case AiType::CONFUSED_MONSTER:
	{
		ai = std::make_shared<ConfusedMonsterAi>(0, nullptr);
		break;
	}

	}

	if (ai != nullptr)
	{
		ai->load(zip);
	}
	else
	{
		std::cout << "Error: Ai::create() - ai is null" << std::endl;
		exit(-1);
	}

	return ai; // TODO: don't return nullptr
}

//==ConfusedMonsterAi==
ConfusedMonsterAi::ConfusedMonsterAi(int nbTurns, std::shared_ptr<Ai> oldAi) noexcept : nbTurns(nbTurns), oldAi(oldAi) {}

void ConfusedMonsterAi::update(Actor& owner)
{
	const gsl::not_null<TCODRandom*> rng = TCODRandom::getInstance();
	int dx = rng->getInt(-1, 1);
	int dy = rng->getInt(-1, 1);

	if (dx != 0 || dy != 0)
	{
		const int destx = owner.posX + dx;
		const int desty = owner.posY + dy;

		if (game.map != nullptr)
		{
			if (game.map->can_walk(desty, destx))
			{
				owner.posX = destx;
				owner.posY = desty;
			}
			else
			{
				std::shared_ptr<Actor> actor = game.get_actor(destx, desty);
				if (actor)
				{
					owner.attacker->attack(owner, *actor);
				}
			}
		}
		else
		{
			std::cout << "Error: update() called on actor with no map." << std::endl;
			exit(-1);
		}
	}

	nbTurns--;
	if (nbTurns == 0)
	{
		owner.ai = oldAi;
	}
}

void ConfusedMonsterAi::load(TCODZip& zip)
{
	nbTurns = zip.getInt();
	oldAi = Ai::create(zip);
}

void ConfusedMonsterAi::save(TCODZip& zip)
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
//====

// end of file: Ai.cpp
