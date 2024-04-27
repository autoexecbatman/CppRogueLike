// file: Pickable.cpp
#include <gsl/util>
#include <vector>

#include "Colors.h"
#include "Actor.h"
#include "Game.h"
#include "Pickable.h"
#include "Healer.h"
#include "LightningBolt.h"
#include "Fireball.h"
#include "Confuser.h"
#include "Container.h"
#include "AiMonsterConfused.h"
#include "LongSword.h"
#include "Dagger.h"

//==PICKABLE==
bool Pickable::pick(std::unique_ptr<Actor> owner, const Actor& wearer)
{
	if (wearer.container && wearer.container->add(std::move(owner)))
	{
		// remove nullptrs from the actors vector
		game.actors.erase(std::remove_if(game.actors.begin(), game.actors.end(), [](const auto& a) noexcept { return !a; }), game.actors.end());

		return true;
	}

	return false;
}

void Pickable::drop(std::unique_ptr<Actor> owner, Actor& wearer)
{
	if (&wearer != game.player) { return; }
	if (wearer.container)
	{
		owner->posX = wearer.posX;
		owner->posY = wearer.posY;
		wearer.container->remove(std::move(owner));
	}
}

bool Pickable::use(Actor& owner, Actor& wearer)
{
	if (wearer.container)
	{
		wearer.container->inventoryList.erase(
			std::remove_if(
				wearer.container->inventoryList.begin(),
				wearer.container->inventoryList.end(),
				[&owner](const std::unique_ptr<Actor>& actor) { return actor.get() == &owner; }
		)
			, wearer.container->inventoryList.end());

		return true;
	}
	return false;
}

std::unique_ptr<Pickable> Pickable::create(TCODZip& zip)
{
	const PickableType type = (PickableType)zip.getInt();
	std::unique_ptr<Pickable> pickable = nullptr;

	switch (type)
	{

	case PickableType::HEALER:
	{
		pickable = std::make_unique<Healer>(0);
		break;
	}

	case PickableType::LIGHTNING_BOLT:
	{
		pickable = std::make_unique<LightningBolt>(0, 0);
		break;
	}

	case PickableType::CONFUSER:
	{
		pickable = std::make_unique<Confuser>(0, 0);
		break;
	}

	case PickableType::FIREBALL:
	{
		pickable = std::make_unique<Fireball>(0, 0);
		break;
	}

	case PickableType::LONGSWORD:
	{
		pickable = std::make_unique<LongSword>(0, 0);
		break;
	}

	case PickableType::DAGGER:
	{
		pickable = std::make_unique<Dagger>(0, 0);
		break;
	}


	}
	if (!pickable) {
		game.log("Pickable create() failed. No pickable type found.");
		game.err("Pickable create() failed. No pickable type found.");
		return nullptr;
	}
	pickable->load(zip);

	return pickable;
}

// end of file: Pickable.cpp
