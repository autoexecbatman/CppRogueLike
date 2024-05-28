// file: Pickable.cpp
#include <gsl/util>
#include <vector>

#include "Pickable.h"
#include "../Colors/Colors.h"
#include "../Game.h"
#include "Actor.h"
#include "Confuser.h"
#include "Container.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/Fireball.h"
#include "../ActorTypes/LongSword.h"
#include "../ActorTypes/Dagger.h"
#include "../Ai/AiMonsterConfused.h"

//==PICKABLE==
bool Pickable::pick(std::unique_ptr<Item> owner, const Creature& wearer)
{
	if (wearer.container && wearer.container->add(std::move(owner)))
	{
		// remove nullptrs from the actors vector
		game.actors.erase(std::remove_if(game.actors.begin(), game.actors.end(), [](const auto& a) noexcept { return !a; }), game.actors.end());

		return true;
	}

	return false;
}

void Pickable::drop(std::unique_ptr<Item> owner, const Creature& wearer)
{
	if (wearer.container)
	{
		owner->position = wearer.position;
		wearer.container->remove(std::move(owner));
	}
}

bool Pickable::use(Item& owner, Creature& wearer)
{
	if (wearer.container)
	{
		wearer.container->inv.erase(
			std::remove_if(
				wearer.container->inv.begin(),
				wearer.container->inv.end(),
				[&owner](const std::unique_ptr<Item>& actor) { return actor.get() == &owner; }
		)
			, wearer.container->inv.end());

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
