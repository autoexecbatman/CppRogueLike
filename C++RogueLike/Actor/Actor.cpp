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
#include "../Items.h"

//====
Actor::Actor(Vector2D position, ActorData data)
	:
	position(position),
	actorData(data)
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

	// this block assigns checks if the actor has a component
	/*const bool hasPickable{ gsl::narrow_cast<bool>(zip.getInt()) };*/
	
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
	if (hasAttacker)
	{
		attacker = std::make_unique<Attacker>("");
		attacker->load(zip);
	}
	if (hasDestructible)
	{
		destructible = Destructible::create(zip);
	}
	if (hasAi)
	{
		ai = Ai::create(zip);
	}
	if (hasContainer)
	{
		container = std::make_unique<Container>(0);
		container->load(zip);
	}
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

	zip.putInt(attacker != nullptr);
	zip.putInt(destructible != nullptr);
	zip.putInt(ai != nullptr);

	if (attacker) attacker->save(zip);
	if (destructible) destructible->save(zip);
	if (ai) ai->save(zip);
	if (container) container->save(zip);
}

void Creature::pick()
{
	auto is_null = [](auto&& i) { return !i; };
	for (auto& i : game.container->inv)
	{
		if (i)
		{
			if (position == i->position)
			{
				if (container->add(std::move(i)))
				{
					std::erase_if(game.container->inv, is_null);
				}
			}
		}
	}
}

void Creature::drop()
{
	auto is_null = [](const auto& i) { return !i; };
	for (auto& i : container->inv)
	{
		if (i)
		{
			i->position = position;
			if (game.container->add(std::move(i)))
			{
				std::erase_if(container->inv, is_null);
			}
		}
	}
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
}

// the actor render function with color
void Actor::render() const noexcept
{
	if (is_visible())
	{
		attron(COLOR_PAIR(actorData.color));
		mvaddch(position.y, position.x, actorData.ch);
		attroff(COLOR_PAIR(actorData.color));
	}
}

void Creature::equip(Item& item)
{
	item.add_state(ActorState::IS_EQUIPPED);
	weaponEquipped = item.actorData.name;
}

void Creature::unequip(Item& item)
{
	item.remove_state(ActorState::IS_EQUIPPED);
	weaponEquipped = "None";
}

// check if the actor is visible
bool Actor::is_visible() const noexcept
{
	return (!has_state(ActorState::FOV_ONLY) && game.map->is_explored(position)) || game.map->is_in_fov(position);
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

// a function to get the Chebyshev distance from an actor to a specific tile of the map
int Actor::get_tile_distance(Vector2D tilePosition) const noexcept
{
	// using chebyshev distance
	const int distance = std::max(abs(position.x - tilePosition.x), abs(position.y - tilePosition.y));

	mvprintw(10, 0, "Distance: %d", distance);

	return distance;
}

//==Item==
Item::Item(Vector2D position, ActorData data) : Object(position, data) {};

void Item::load(TCODZip& zip)
{
	// this block assigns checks if the actor has a component
	const bool hasPickable{ gsl::narrow_cast<bool>(zip.getInt()) };
	const bool hasContainer{ gsl::narrow_cast<bool>(zip.getInt()) };

	// this block assigns the values from the zip file to the actor's components if they exist.
	if (hasPickable) { pickable = Pickable::create(zip); }
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

	zip.putInt(pickable != nullptr);
	if (pickable) pickable->save(zip);
}

// end of file: Actor.cpp
