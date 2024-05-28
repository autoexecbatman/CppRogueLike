// file: Actor.cpp
#include <iostream>
#include <string>
#include <algorithm>
#include <math.h>
#include <memory>
#include <gsl/gsl>

#include <curses.h>
#pragma warning (push, 0)
#include <libtcod.h>
#pragma warning (pop)

#include "../Game.h"
#include "../Ai/Ai.h"
#include "Actor.h"
#include "Attacker.h"
#include "Destructible.h"
#include "Pickable.h"
#include "Container.h"

//====
Actor::Actor(Vector2D position, ActorData data, ActorFlags flags)
	:
	position(position),
	actorData(data),
	flags(flags)
{}

//====
void Actor::load(TCODZip& zip)
{
	// this block assigns the values from the zip file to the actor
	position.x = zip.getInt();
	position.y = zip.getInt();

	actorData.ch = zip.getInt();
	actorData.color = zip.getInt();
	actorData.name = _strdup(zip.getString());

	flags.blocks = zip.getInt();

	// this block assigns checks if the actor has a component
	const bool hasPickable{ gsl::narrow_cast<bool>(zip.getInt()) };
	
	// this block assigns the values from the zip file to the actor's components if they exist.
	
}

void Creature::load(TCODZip& zip)
{
	// this block assigns checks if the actor has a component
	const bool hasAttacker{ gsl::narrow_cast<bool>(zip.getInt()) };
	const bool hasAi{ gsl::narrow_cast<bool>(zip.getInt()) };
	const bool hasDestructible{ gsl::narrow_cast<bool>(zip.getInt()) };
	const bool hasContainer{ gsl::narrow_cast<bool>(zip.getInt()) };

	// this block assigns the values from the zip file to the actor's components if they exist.
	if (hasAttacker) { attacker = std::make_unique<Attacker>(0, 0, 0); attacker->load(zip); }
	if (hasDestructible) { destructible = Destructible::create(zip); }
	if (hasAi) { ai = Ai::create(zip); }
	if (hasContainer) { container = std::make_unique<Container>(0); container->load(zip); }
}

void Item::load(TCODZip& zip)
{
	// this block assigns checks if the actor has a component
	const bool hasPickable{ gsl::narrow_cast<bool>(zip.getInt()) };
	const bool hasContainer{ gsl::narrow_cast<bool>(zip.getInt()) };

	// this block assigns the values from the zip file to the actor's components if they exist.
	if (hasPickable) { pickable = Pickable::create(zip); }
}

void Creature::save(TCODZip& zip)
{
	// Add a debug log to display the actor name
	game.log("Saving actor: " + actorData.name);

	zip.putInt(position.x);
	zip.putInt(position.y);
	zip.putInt(actorData.ch);
	zip.putInt(actorData.color);
	zip.putString(actorData.name.c_str());
	zip.putInt(flags.blocks);

	zip.putInt(attacker != nullptr);
	zip.putInt(destructible != nullptr);
	zip.putInt(ai != nullptr);

	if (attacker) attacker->save(zip);
	if (destructible) destructible->save(zip);
	if (ai) ai->save(zip);
}

void Creature::pick()
{
	// use alias for long name: game.items->inventoryList
	auto& inv = game.container->inv;
	for (auto& i : inv)
	{
		if (i)
		{
			if (position == i->position)
			{
				// add item to inventory
				if (container->add(std::move(i)))
				{
					inv.erase(std::remove(inv.begin(), inv.end(), nullptr), inv.end());
				}
			}
		}
	}
}

void Item::save(TCODZip& zip)
{
	// Add a debug log to display the actor name
	game.log("Saving actor: " + actorData.name);

	zip.putInt(position.x);
	zip.putInt(position.y);
	zip.putInt(actorData.ch);
	zip.putInt(actorData.color);
	zip.putString(actorData.name.c_str());
	zip.putInt(flags.blocks);

	zip.putInt(pickable != nullptr);

	if (pickable) pickable->save(zip);
}

void Actor::save(TCODZip& zip)
{
	// Add a debug log to display the actor name
	game.log("Saving actor: " + actorData.name);

	zip.putInt(position.x);
	zip.putInt(position.y);
	zip.putInt(actorData.ch);
	zip.putInt(actorData.color);
	zip.putString(actorData.name.c_str());
	zip.putInt(flags.blocks);

}

// the actor render function with color
void Actor::render() const noexcept
{
	attron(COLOR_PAIR(actorData.color));
	mvaddch(position.y, position.x, actorData.ch);
	attroff(COLOR_PAIR(actorData.color));
}

void Creature::equip(Actor& item)
{
	item.flags.isEquipped = true;
	weaponEquipped = item.actorData.name;
}

void Creature::unequip(Actor& item)
{
	item.flags.isEquipped = false;
	weaponEquipped = "None";
}

bool Actor::is_visible() const noexcept
{
	return (!flags.fovOnly && game.map->is_explored(position)) || game.map->is_in_fov(position);
}

// the actor update
void Creature::update()
{
	// if the actor has an ai then update the ai
	if (ai)
	{
		ai->update(*this);
	}
}

// a function to get the distance from an actor to a specific tile of the map
int Actor::get_tile_distance(Vector2D tilePosition) const noexcept
{
	// using chebyshev distance
	const int distance = std::max(abs(position.x - tilePosition.x), abs(position.y - tilePosition.y));

	mvprintw(10, 0, "Distance: %d", distance);

	return distance;
}

// end of file: Actor.cpp
