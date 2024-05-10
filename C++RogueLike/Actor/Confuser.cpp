#include "Confuser.h"
#include "Actor.h"
#include "../Game.h"
#include "../Ai/AiMonsterConfused.h"

//==Confuser==
Confuser::Confuser(int nbTurns, int range) noexcept : nbTurns(nbTurns), range(range) {}

bool Confuser::use(Actor& owner, Actor& wearer)
{
	int x{ 0 }, y{ 0 };

	if (!game.pick_tile(&x, &y, 0))
	{
		return false;
	}

	const std::unique_ptr<Actor>& actor = game.get_actor(x, y);

	if (!actor)
	{
		return false;
	}

	// replace the monster's AI with a confused one; 
	// after <nbTurns> turns the old AI will be restored
	auto confusedAi = std::make_unique<AiMonsterConfused>(nbTurns, std::move(actor->ai));
	actor->ai = std::move(confusedAi);

	game.message(WHITE_PAIR, std::format("as he starts to stumble around!"), true);
	game.message(WHITE_PAIR, std::format("The eyes of the {} look vacant,", actor->name), true);


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