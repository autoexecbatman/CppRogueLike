#include "Confuser.h"
#include "Actor.h"
#include "Game.h"
#include "AiMonsterConfused.h"

//==Confuser==
Confuser::Confuser(int nbTurns, int range) : nbTurns(nbTurns), range(range) {}

bool Confuser::use(Actor& owner, Actor& wearer)
{
	game.gui->log_message(WHITE_PAIR, "Left-click an enemy to confuse it,\nor right-click to cancel.");

	int x, y;

	if (!game.pick_tile(&x, &y, range))
	{
		return false;
	}

	auto actor = game.get_actor(x, y);

	if (!actor)
	{
		return false;
	}

	// replace the monster's AI with a confused one; 
	// after <nbTurns> turns the old AI will be restored
	/*ConfusedMonsterAi* confusedAi = new ConfusedMonsterAi(nbTurns, actor->ai);*/
	auto confusedAi = std::make_shared<AiMonsterConfused>(nbTurns, actor->ai);
	actor->ai = confusedAi;
	game.gui->log_message(WHITE_PAIR, "The eyes of the %s look vacant,\nas he starts to stumble around!", actor->name);


	return Pickable::use(owner, wearer);
}

void Confuser::load(TCODZip& zip)
{
	nbTurns = zip.getInt();
	range = zip.getInt();
}

void Confuser::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::CONFUSER));
	zip.putInt(nbTurns);
	zip.putInt(range);
}