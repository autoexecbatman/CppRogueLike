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

//==PICKABLE==
bool Pickable::pick(Actor& owner, const Actor& wearer)
{
	if (wearer.container && wearer.container->add(owner))
	{
		// TODO : Fix this to remove the item from the map after it has been picked up.
		/*game.actors.erase(
		std::remove(game.actors.begin(), game.actors.end(), owner),
		game.actors.end()
		);*/

		game.gui->log_message( // displays the message
			COLOR_WHITE,
			"%s picks up a %s.",
			wearer.name.c_str(),
			owner.name.c_str()
		);
		
		// first iterate through the list of actors
		for (auto actor : game.actors)
		{
			// if the actor is the owner of the item
			if (actor.get() == &owner)
			{
				// remove the item from the list of actors
				//game.actors.erase(
				//	std::remove(game.actors.begin(), game.actors.end(), actor),
				//	game.actors.end()
				//);
				
				std::erase(game.actors, actor);

				//actor->ch = '.';
				//actor->col = WHITE_PAIR;

				break;
			}
		} 

		return true;
	}

	return false;
}

void Pickable::drop(Actor& owner, Actor& wearer)
{
	if (&wearer != game.player.get())
	{
		return;
	}

	if (wearer.container)
	{
		wearer.container->remove(owner);

		// Create a new Actor for the dropped item
		auto dropped_item = std::make_shared<Actor>(owner);

		// Initialize the new Actor with the attributes of the dropped item
		dropped_item->posX = wearer.posX;
		dropped_item->posY = wearer.posY;
		dropped_item->ch = owner.ch;  // Use the character symbol of the dropped item
		dropped_item->col = owner.col;  // Use the color of the dropped item

		// Add the new Actor to game.actors
		game.actors.push_back(dropped_item);
		game.send_to_back(*dropped_item);
	}
}

bool Pickable::use(Actor& owner, Actor& wearer)
{
	std::clog << "Using item" << std::endl;
	if (wearer.container)
	{
		wearer.container->remove(owner);
		/*delete owner;*/
		return true;
	}
	return false;
}

std::shared_ptr<Pickable> Pickable::create(TCODZip& zip)
{
	PickableType type = (PickableType)zip.getInt();
	std::shared_ptr<Pickable> pickable = nullptr;

	switch (type)
	{

	case PickableType::HEALER:
	{
		pickable = std::make_shared<Healer>(0);
		break;
	}

	case PickableType::LIGHTNING_BOLT:
	{
		pickable = std::make_shared<LightningBolt>(0, 0);
		break;
	}

	case PickableType::CONFUSER:
	{
		pickable = std::make_shared<Confuser>(0, 0);
		break;
	}

	case PickableType::FIREBALL:
	{
		pickable = std::make_shared<Fireball>(0, 0);
		break;
	}

	}

	pickable->load(zip);

	return pickable;
}

// end of file: Pickable.cpp
